#include <chrono>
#include <cmath>
#include <iostream>
#include <mpi.h>
#include <vector>
#include "utils/constants.h"
#include "utils/diag_info.h"
#include "utils/mpi_myinfo.h"
#include "utils/square_matrix.h"

/**
 *
 * @param vec_row[in] = the row vector
 * @param vec_col[in] = the col vector
 * @param length[in] = length of the two vectors
 * @param temp[out] = where to store the computation
 */
void LocalDotProd(const std::vector<double>& vec_row, const std::vector<double>& vec_col, const u64 length, double& temp) {
    temp = 0.0;
    for (u64 i = 0; i < length; ++i)
        temp += vec_row[i] * vec_col[i];
}

/**
 * @brief Sets ScatterV_arrays (row_displs, col_displs, counts)
 * @param diag[in] = obj containing informations on the diagonal
 * @param mtx[in] = obj containing the matrix (if Master), or a dummy buffer (if Worker)
 * @param elem_row[in] = row index of the element
 * @param elem_col[in] = col index of the element
 * @param my_stuff[out] = obj containing the arrays to modify
 */
inline auto SetScatterArrays(const DiagInfo &diag, const SquareMtx &mtx, const u64 elem_row, const u64 elem_col, MyInfo &my_stuff)
        -> void {

    // Determine first elem of row_vec
    const u64 row_vec_row = elem_row;
    const u64 row_vec_col = elem_row;

    // Determine fist elem col_vector
    // In reality we use a row vector in the down-part of the Matrix where we store
    // the values flipped. TODO GIVE A BETTER EXPLAINATION
    const u64 col_vec_row = elem_col;
    const u64 col_vec_col = elem_row + 1;

    // Filling arrays
    for (int id = 0; id < my_stuff.num_processes; ++id) {

        // Filling row_vec
        // We move by column
        u64 start_range = row_vec_col + (id * diag.mpi_chunk_size);
        if (start_range >= elem_col) {  // Checking Out of Bounds
            my_stuff.SetOutOfBounds(id);
            break;
        }
        u64 end_range = (start_range + diag.mpi_chunk_size) - 1;
        if (end_range >= elem_col) // Checking Out of Bounds
            end_range = elem_col - 1;
        my_stuff.counts[id] = (static_cast<int>((end_range - start_range)) + 1);
        my_stuff.row_displs[id] =
                static_cast<int>(mtx.GetIndex(row_vec_row, start_range));

        // Determining range for snd_vec
        start_range = col_vec_col + (id * diag.mpi_chunk_size);
        my_stuff.col_displs[id] =
                static_cast<int>(mtx.GetIndex(col_vec_row, start_range));
    }
}


/**
 * @brief Execute a parallel computation of an element of the matrix.
 *        Each Process determines its chunk of elements and gets it through ScatterV.
 *        Then they apply separately the DotProduct, returning the computed value using Reduce.
 *        Finally the Master Process will get the sum of all local DotProduct, apply to it the
 *        cube_root and then store it in the matrix.
 * @param num_elem[in] = number of the elements in regards of the diagonal where it is stored.
 *                       NOTE: the elements start at position 1.
 * @param diag = obj containing diagonal informations
 * @param my_stuff = obj containing arrays and params of the process
 * @param mtx = for the Master it contains the matrix to compute
 *              for the Workers it contains a dummy buffer for the ScatterV.
 */
inline void ComputeElem(const u64 num_elem, const DiagInfo & diag, MyInfo &my_stuff, const SquareMtx & mtx) {

    // [ALL] Determining row and col of the element
    const u64 row = num_elem - 1;
    const u64 col = row + diag.num;
    double temp{0};

    // [ALL] Filling count and displs arrays
    SetScatterArrays(diag, mtx, row, col, my_stuff);

    // [ALL] Determining if I have to do a LocalDotProduct Computation i.e. if my cell in counts is > 0
    if (my_stuff.MyCount() != 0) {
        // [ALL] Retrieving row vector
        MPI_Scatterv(mtx.data->data(), my_stuff.counts.data(), my_stuff.row_displs.data(), MPI_DOUBLE,
                     my_stuff.local_row.data(), my_stuff.MyCount(), MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);

        // [ALL] Retrieving col vector
        MPI_Scatterv(mtx.data->data(), my_stuff.counts.data(), my_stuff.col_displs.data(), MPI_DOUBLE,
                     my_stuff.local_col.data(), my_stuff.MyCount(), MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);

        // [ALL] Executing local DotProd
        LocalDotProd(my_stuff.local_row, my_stuff.local_col, my_stuff.MyCount(), temp);
    }

    // [ALL] Sending/Receiving the results
    double total_sum;
    MPI_Reduce(&temp, &total_sum, 1, MPI_DOUBLE, MPI_SUM, MASTER_RANK, MPI_COMM_WORLD);

    // [MASTER] Storing final result
    if (my_stuff.AmIMaster()) {
        // Applying cube_root to result
        total_sum = std::cbrt(total_sum);
        // Storing result in the matrix
        mtx.SetValue(row, col, total_sum);
        mtx.SetValue(col, row, total_sum);
    }
}

/**
 * @brief Execute a Parallel execution of the Wavefront Pattern using MPI
 * @param mtx_length = length of the matrix
 * @param diag = object storing information regarding the current diagonal to compute
 * @param my_stuff = contains arrays and params of the process
 * @return
 */
inline SquareMtx* MPIWavefront(const u64 mtx_length, DiagInfo& diag, MyInfo &my_stuff) {

    SquareMtx* mtx;
    // [MASTER] Initialize the matrix
    if (my_stuff.AmIMaster())
        mtx = new SquareMtx{mtx_length};
    // [WORKERS] Create dummy matrix
    else
        mtx = new SquareMtx{1};

    // [ALL] Start WaveFront Computation
    while (diag.num < mtx_length) {
        for (u64 elem = 1; elem <= diag.length; ++elem) {
            ComputeElem(elem, diag, my_stuff, *mtx);
        }
        diag.PrepareNextDiagonal();
    }

    // [MASTER] Return matrix
    if(my_stuff.AmIMaster())
        return mtx;

    // [WORKER] Return nothing (nullptr)
    return nullptr;
}

/**
 * @brief A Parallel Wavefront Computation using MPI.
 * @param[in] argv[0] = the length of a row (or column) of the square matrix.
 * @param[in] argc = number of cmd arguments.
 */
int main(int argc, char *argv[]) {
    // Setting mtx_length Parameters
    u64 mtx_length{default_length};
    if (argc == 2) // If the user as passed its own lenght
                   // for the matrix use it instead
        mtx_length = std::stoul(argv[1]);

    // [ALL] Initialize MPI
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        std::cerr << "in MPI_Init" << std::endl;
        return EXIT_FAILURE;
    }

    // [ALL] Setting up base params
    int my_rank{0};
    int num_processes{0};
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    MyInfo my_stuff{my_rank, num_processes, mtx_length - 1};
    DiagInfo diag{mtx_length, num_processes};

    if (my_rank == MASTER_RANK)
        std::cout << "Starting MPI_WaveFront Computation with:\n"
                  << "num_processes = " << my_stuff.num_processes << "\n"
                  << "mtx.length = " << mtx_length << std::endl;


    // [ALL] Start Computation
    const auto start = std::chrono::steady_clock::now();
    const SquareMtx * mtx = MPIWavefront(mtx_length, diag, my_stuff);

    // [MASTER] Printing duration
    if(my_stuff.AmIMaster()) {
        const auto end = std::chrono::steady_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Time taken for MPI version: " << duration.count() << " milliseconds" << std::endl;
    }

    // [ALL] Deleting SquareMatrix pointer
    delete mtx;

    // [ALL] Closing MPI Communication and ending the program
    MPI_Finalize();
    return EXIT_SUCCESS;
}
