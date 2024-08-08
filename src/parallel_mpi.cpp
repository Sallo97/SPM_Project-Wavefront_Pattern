#include <cmath>
#include <iostream>
#include <mpi.h>
#include <vector>
#include "utils/diag_info.h"
#include "utils/square_matrix.h"
#include "utils/elem_info.h"
#define EOS_MESSAGE (-1)
#define MASTER_RANK 0
using double_vec = std::vector<std::vector<double>>;

/**
 * @brief Does a DotProduct Computation given the row and col.
 * @param[in] row_vector = the row to apply the DotProduct
 * @param[in] col_vector = the col to apply the DotProduct
 * @param[out] result = where to store the computation
 */
void DotProduct(const std::vector<double>& row_vector,
                const std::vector<double>& col_vector,
                double& result) {
    result = 0.0;
    for(int i = 0; i < row_vector.size(); ++i)
        result += row_vector[i] * col_vector[i];
}

/**
 * @brief Does a DotProduct Computation for a Sequential case.
 * @param[in] mtx = reference of the matrix where to apply the computation
 * @param[in] elem = contains informations regarding the element to compute
 * @param[in] length = size of the "vectors"
 * @param[out] result = where to store the computation
 */
void DotProduct(const SquareMtx& mtx,
                int length,
                const ElemInfo& elem,
                double& result) {
    result = 0.0;
    for(int i = 0; i < length; ++i)
        result += mtx.data[elem.vec1_idx + i] *
                                mtx.data[elem.vec2_idx + 1];
}

/**
    * @brief Set the snd_count and dipls array accordingly
    * @param[in] diag_length = the length of the diagonal where the elem is from.
    * @param[in] num_processes = the number of processes.
    * @param[in] idx_fst_elem = the index of the mtx of the first elem in the array to send
    * @param[in] snd_count = the array snd_count to set
    * @param[in] displs = the array dipls to set
 */
void SetScatterArrays(const int& diag_length,
                      const int& num_processes,
                      int idx_fst_elem,
                      std::vector<int>&snd_count,
                      std::vector<int>&displs)
{
    // Setting common params
    int chunk_size = std::ceil(diag_length / num_processes);

    // Setting snd_count and displs
    for(int i = 0; i < num_processes; ++i) {
        displs[i] = idx_fst_elem + (i * chunk_size);
        snd_count[i] = chunk_size;
    }
    if(diag_length % num_processes != 0)
        snd_count[snd_count.size() - 1] = diag_length % num_processes;
}

/**
 * @brief Handles the Parallel Communication for computing the DotProduct
 * @param[in] num_processes
 * @param[in] my_rank
 * @param[out] local_sum
 * @param[out] total_sum
 * @param[in] diag_info
 * @param[in] elem
 * @param[in] mtx
 * @param[in] snd_count
 * @param[in] displs
 * @param[in] local_row
 * @param[in] local_col
 */
void ParallelComputation(const int& num_processes, const int& my_rank,
                         double& local_sum, double& total_sum,
                         const DiagInfo& diag_info, const ElemInfo& elem,
                         const SquareMtx& mtx,
                         std::vector<int>& snd_count,
                         std::vector<int>& displs,
                         std::vector<double>& local_row,
                         std::vector<double>& local_col
                         ) {
    // Send/Receive Row Vector
    SetScatterArrays(diag_info.length, num_processes,
     elem.vec1_idx, snd_count, displs);
    MPI_Scatterv(mtx.data.data(), snd_count.data(), displs.data(),
                 MPI_DOUBLE, local_row.data(), snd_count[my_rank],
                 MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);

    // Send/Receive Col. Vector
    SetScatterArrays(diag_info.length, num_processes,
                     elem.vec2_idx, snd_count, displs);
    MPI_Scatterv(mtx.data.data(), snd_count.data(), displs.data(),
                 MPI_DOUBLE, local_col.data(), snd_count[my_rank],
                 MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);

    // [COMMON] DotProduct only on local row and local col
    DotProduct(local_row, local_col, local_sum);

    // [COMMON] Call Reduce
    MPI_Reduce(&local_sum, &total_sum, 1, MPI_DOUBLE, MPI_SUM,
            MASTER_RANK, MPI_COMM_WORLD);
}

/**
 * @brief Handles the Wavefront Computation. It is called by both Master process and the Emitters.
 * @param my_rank
 * @param num_processes
 * @param limit
 * @param row_length
 * @param diag_info
 * @param mtx
 * @param local_sum
 * @param local_row
 * @param local_col
 * @param snd_count
 * @param displs
 */
void WaveFrontComputation(const int& my_rank, const int& num_processes,
                          const int& limit, const int& row_length,
                          double& local_sum, DiagInfo diag_info,
                          SquareMtx& mtx,
                          std::vector<double>& local_row,
                          std::vector<double>& local_col,
                          std::vector<int>& snd_count,
                          std::vector<int>& displs) {

    // [COMMON] Compute each diagonal of the matrix
    double total_sum = 0.0;
    while(!diag_info.IsLastDiag()) {
        diag_info.UpdateDiag();

        // [COMMON] Computing the current diagonal
        for(int i = 0; i < diag_info.length; ++i) {
            ElemInfo elem{diag_info,i, row_length};
            int vectors_length = diag_info.current_diag; //length of row and col of DotProduct

            // [COMMON] Compute current element
            if (vectors_length > limit) {         // [COMMON]
                                                  // PARALLEL DOTPRODUCT
                ParallelComputation(num_processes, my_rank, local_sum,
                                    total_sum, diag_info, elem,
                                    mtx, snd_count, displs,
                                    local_row, local_col);
            }
            else if(my_rank == MASTER_RANK) {     // [ONLY MASTER]
                                                  // SEQUENTIAL DOTPRODUCT
                DotProduct(mtx, vectors_length, elem, total_sum);
            }

            // [ONLY MASTER] Update matrix
            if(my_rank == MASTER_RANK)
                mtx.SetValue(elem.row, elem.col, std::cbrt(total_sum));
        }
    }
}


/**
 * @brief A Parallel Wavefront Computation using MPI.
 * @param[in] argv[0] = the number of processes spawned by MPI
 * @param[in] argv[1] = the length of a row (or column) of the square matrix.
 * @param[in] argv[2] = the limit of the chunk_size for the
 * @param[in] argc = number of cmd arguments.
 */
int main(int argc, char* argv[]) {
    // Setting base paramters
    int row_length{4096};  // Default length of the square matrix
    int limit{64};  // Default length of the chunk_limit for parallelize
    int my_rank{0};    int num_processes{0};

    // Initialize MPI and Setting MPI parameters
    if ( MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        std::cerr<<"in MPI_Init"<<std::endl;
        return EXIT_FAILURE;
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    // Checking CMD arguments
    if (argc >= 2)
        row_length = std::stoi(argv[1]);
    if (argc >= 3)
        limit = std::stoi(argv[2]);

    // Setting up common buffers
    std::vector<double> local_row(row_length, 0.0);
    std::vector<double> local_col(row_length, 0.0);
    std::vector<int> snd_count(row_length-1, 0.0);
    std::vector<int> displs(row_length-1, 0.0);
    double local_sum{0.0};
    DiagInfo diag_info{row_length};

    // [ONLY MASTER] Initialize Matrix
    SquareMtx mtx;
    // [ONLY EMITTER] Initializing the matrix
    if(my_rank == MASTER_RANK) {
        mtx.InitializeMatrix(row_length);
    }

    // Starting WaveFront Computation
    WaveFrontComputation(my_rank, num_processes, limit,
                         row_length,local_sum,
                         diag_info,
                         mtx,
                         local_row,
                         local_col,
                         snd_count,
                         displs);

    // Exiting the application
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return EXIT_SUCCESS;

}