/**
* @file parallel_fastflow.cpp
 * @brief This code is a parallel implementation of the Wavefront computation using the FastFlow library.
 * @details It initialize a matrix of dimension NxN (parameter given as input).
 *          All values are set to 0 except the one in the major diagonal s.t.:
 *          ∀i in [0, N].mtx[i][i] = (i + 1) / n.
 *          Then all elements in the upper part of the major diagonal are computed s.t.:
 *          mtx[i][j] = square_cube( dot_product(v_m, v_m+k) ).
 *          This is parallelized by using a farm with feedback channels.
 *          The Emitter send to the worker a chunck of elements of the current diagonal to compute.
 *          The Workers compute them and send a "done" message to the Emitter.
 *          When all elements of the diagonal have been computer, the Emitter repeat
 *          the process for the next diagonal.
 * @author Salvatore Salerno
 */
#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#define EOS_MESSAGE (-1)
#define MASTER_RANK 0
using double_vec = std::vector<double>;


// Struct giving informations on an element of the matrix
// We use it in the Master to have a more clear algorithm
struct ElemInfo {
    int idx_row;
    int idx_col;
};

// Struct for our Square Matrix
// The matrix is in reality two vectors:
// rows which stores all the row of the matrix
// cols which stores all the cols of the matrix
// This is done to better optimize the parallelization
// of the dotProduct, losing in memory efficency
struct VecMatrix {
    /**
     * @brief Creates the vectors of rows and columns
     *        each contaning the values of the first
     *        major diagonal s.t.
     *        ∀m in [0, n].mtx[m][m] = (m + 1) / n.
     * @param vec_length = length of the vectors
     */
    explicit VecMatrix(int vec_length) {
        // Initializing rows_vector
        rows_vector.reserve(vec_length);
        cols_vector.reserve(vec_length);
        for (int i = 0; i < vec_length; ++i) {
            double val = static_cast<double>(i + 1) / static_cast<double>(vec_length);
            rows_vector.emplace_back(1, val); // Each sub-vector starts
            cols_vector.emplace_back(1, val); // length 1 and value (i+1)/n
        }
    }

    /**
        * @brief Returns a reference to the row
        *        of interest.
        * @param[in] idx = the idx of the row in the matrix
    */
    std::vector<double>& GetRow(int idx) {
        return rows_vector[idx];
    }

    /**
        * @brief Returns a reference to the column
        *        of interest.
        * @param[in] idx = the idx of the column in the matrix
    */
    std::vector<double>& GetCol(int idx) {
        return cols_vector[idx];
    }

    /**
        * @brief Puts an element of the matrix in the
        *        appropriate row and column for reuse
        *        in a following DotProduct
        * @param[in] x = the element to put.
        * @param[in] val = the value to store
    */
    void PutElement(ElemInfo x, double val) {
        rows_vector[x.idx_row].push_back(val);
        cols_vector[x.idx_col].push_back(val);
    }

    // Parameters
    std::vector<double_vec>rows_vector;
    std::vector<double_vec>cols_vector;
};

/**
 * @brief Apply the DotProduct sequentially.
 * @param[in] length = length of the vectors to multiply
 * @param[in] mtx = reference to the matrix
 * @param[out] x = the element to compute
 */
double SequentialDotProduct(int& length, VecMatrix& mtx, ElemInfo& x) {
    // Setting up
    std::vector<double>& row = mtx.GetRow(x.idx_row);
    std::vector<double>& col = mtx.GetCol(x.idx_col);
    double sum{0.0};

    // Beginning DotProduct
    for(int i = 0; i < length; ++i)
        sum += row[i] * col[i];

    return sum;
}

/**
    * @brief Delegates the DotProduct to the Workers.
    * @param[in] length = length of the vectors to multiply
    * @param[in] numP = number of processes
    * @param[in] mtx = reference to the matrix
    * @param[out] x = the element to compute
*/
double ParallelDotProduct(int& length, int& num_processes, VecMatrix& mtx,
                          ElemInfo x, std::vector<double>& local_row,
                          std::vector<double>& local_col) {
    // Setting up parameters
    std::vector<double>& row = mtx.GetRow(x.idx_row);
    std::vector<double>& col = mtx.GetCol(x.idx_col);
    local_row.resize(length);

    // Send the length to all Workers
    MPI_Bcast(&length, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Scatter the row and column vector
    // TODO Be aware that for the scatter to work #total_elements % num_processes
    MPI_Scatter(row.data(),
                length,
                MPI_DOUBLE,
                local_row.data(),
                length,
                MPI_DOUBLE,
                0,
                MPI_COMM_WORLD);

    MPI_Scatter(col.data(),
                length,
                MPI_DOUBLE,
                local_col.data(),
                length,
                MPI_DOUBLE,
                0,
                MPI_COMM_WORLD);

    // The root process compute its part of the chunk
    double local_sum{0.0};
    for (int i = 0; i < length; ++i)
        local_sum = local_row[i] * local_col[i];

    // Gather results from all processes
    double sum{0.0};
    MPI_Reduce(&local_sum,
               &sum,
               1,
               MPI_DOUBLE,
               MPI_SUM,
               0,
               MPI_COMM_WORLD);
    return sum;
}

/**
    * @brief The main actor in the computation of the Wavefront Pattern.
    *        It computes sequentially each diagonal. The dot_product com-
    *        putation is delegated to the Workers
    * @param[in] row_length = length of a row/column of the matrix to compute
    * @param[in] limit = it determines if the diagonal can be computed sequentially
    *                    by the Emitter or with a ParallelDotProduct.
*/
void MasterJob(int& my_rank, int& num_processes, int& row_length,
                double& local_sum, std::vector<double>& local_row,
                std::vector<double>& local_col) {
    std::cout << "sono il Master" << std::endl;

    // Initializing Matrix
    VecMatrix mtx(row_length);

    // Beginning Wavefront Pattern
    int diag_length{0};
    int limit{64};   // AHAH LIKE SUPERMARIO TODO GIVE IT AS A CMD ARGUMENT
    for (int current_diag = 0; current_diag < row_length; ++current_diag) {
        diag_length = row_length - current_diag;
        for(int k = 0; k < diag_length; ++k) {
            ElemInfo x{k, current_diag + 1 + k};
            double val{0};

            // Computing the element
            if(diag_length < limit) {   // diag_length determines the
                                        // the number of elements of the
                                        // row vector and col vector
                                        // for that element
                val = SequentialDotProduct(diag_length, mtx, x);
            }else {
                val = ParallelDotProduct(diag_length,num_processes,mtx,
                                         x,local_row,local_col);
            }

            // Storing the element in the matrix
            mtx.PutElement(x, std::cbrt(val));
        }
    }

    // Ending Workers
    int end_msg{-1};
    MPI_Bcast(&end_msg, 1, MPI_INT, 0, MPI_COMM_WORLD);
}

//TODO Aggiungere descrizione
void WorkerJob(int& my_rank, int& row_length, double& local_sum,
                std::vector<double>& local_row, std::vector<double>& local_col) {
    std::cout << "sono Worker n" << my_rank << std::endl;
    bool end{false};

    while(!end) {

        // Receiving length
        int length{0};
        MPI_Bcast(&length, 1, MPI_INT, 0, MPI_COMM_WORLD);

        if(length == -1) {
            end = true;
            break;
        }

        // Receiver the row and column vector using Scatter
        std::vector<double> row(1, 0.0);
        std::vector<double> col(1, 0.0);
        MPI_Scatter(row.data(),
            length,
            MPI_DOUBLE,
            local_row.data(),
            length,
            MPI_DOUBLE,
            0,
            MPI_COMM_WORLD);

        MPI_Scatter(col.data(),
            length,
            MPI_DOUBLE,
            local_col.data(),
            length,
            MPI_DOUBLE,
            0,
            MPI_COMM_WORLD);

        // The Worker computes its part of the chunk
        // The root process compute its part of the chunk
        for (int i = 0; i < length; ++i)
            local_sum = local_row[i] * local_col[i];

        double sum{0.0};
        // Send result
        MPI_Reduce(&local_sum,
           &sum,
           1,
           MPI_DOUBLE,
           MPI_SUM,
           0,
           MPI_COMM_WORLD);
    }
}

/**
 * @brief A Parallel Wavefront Computation using MPI.
 * @param[in] argv[1] is the number of processes
 * @param[in] argv[2] is the length of a row (or a column) of the matrix
 * @exception argc < 3
 */
int main(int argc, char* argv[]) {
    // Setting base paramters
    int row_length{4096};  // Default length of the square matrix
    int my_rank{0};    int num_processes{0};

    // Initialize MPI and Setting base parameters
    if ( MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        std::cerr<<"in MPI_Init"<<std::endl;
        return EXIT_FAILURE;
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    // Checking CMD arguments
    if (argc == 2)
        row_length = std::stoi(argv[1]);
    if(!my_rank && (row_length % num_processes)) {
        std::cerr << "ERROR the row length must be a multiple of the number of processes"
                  << std::endl;
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Setting up common buffers
    std::vector<double> local_row(row_length, 0.0);
    std::vector<double> local_col(row_length, 0.0);
    double local_sum{0.0};

    // Call the appropriate job
    if (my_rank == 0)
        MasterJob(my_rank, num_processes, row_length,
                local_sum, local_row, local_col);
    else
        WorkerJob(my_rank, row_length, local_sum,
                    local_row, local_col);

    // Exiting the application
    MPI_Barrier(MPI_COMM_WORLD);    // TODO RIMUOVERE ALLA FINE
    MPI_Finalize();

    return EXIT_SUCCESS;
}