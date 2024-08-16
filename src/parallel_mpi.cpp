#include <__chrono/calendar.h>
#include <cmath>
#include <iostream>
#include <mpi.h>
#include <vector>

#include "utils/constants.h"
#include "utils/ff_diag_info.h"
#include "utils/square_matrix.h"
using double_vec = std::vector<std::vector<double>>;

// TODO DESCRIPTION
inline void SetScatterArrays(u64 num_elem, DiagInfo& diag, SquareMtx& mtx,
                      int num_workers, u64 elem_row, u64 elem_col,
                      vec_int& fst_counts, vec_int& fst_displs,
                      vec_int& snd_counts, vec_int& snd_displs) {


    // Determine first elem of fst_vec
    const u64 fst_vec_row = elem_row;
    const u64 fst_vec_col = elem_row;

    // Determine fist elem snd_vector
    const u64 snd_vec_row = elem_col;
    const u64 snd_vec_col = elem_row + 1;

    std::cout << "fst_vec_row = " << fst_vec_row << " fst_vec_col = " << fst_vec_col << "\n"
              << "snd_vec_row = " << snd_vec_row << " snd_vec_col = " << snd_vec_col << std::endl;

    // Filling arrays
    u64 start_range;    u64 end_range;
    for(int id = 0; id < num_workers; ++id) {
        // Determine range for fst_vec
        start_range = fst_vec_col + (id * diag.chunk_size);
        end_range = (start_range + diag.chunk_size) - 1;
        if(end_range >= elem_col)
            end_range = elem_col - 1;
        fst_counts[id] = (end_range - start_range) + 1;
        fst_displs[id] = mtx.GetIndex(fst_vec_row, start_range);    // TODO CAMBIA TIPO VETTORE IN U64

        // Determining range for snd_vec
        start_range = snd_vec_col + (id * diag.chunk_size);
        end_range = (start_range + diag.chunk_size) - 1;
        if(end_range > elem_col)
            end_range = elem_col ;
        snd_counts[id] = (end_range - start_range) + 1;
        snd_displs[id] = mtx.GetIndex(snd_vec_row, start_range);    // TODO CAMBIA TIPO VETTORE IN U64
    }
}

// TODO DESCRIZIONE
inline void ComputeElem(int my_rank, u64 num_elem, DiagInfo& diag, SquareMtx& mtx,
                      int num_workers,
                      vec_int& row_counts, vec_int& row_displs,
                      vec_int& col_counts, vec_int& col_displs,
                      vec_double& local_row, vec_double& local_col,
                      double& temp, double& total_sum) {

    // Determining row and col of the element
    const u64 row = num_elem - 1;
    const u64 col = row + diag.num;
    if(my_rank == MASTER_RANK)

    // [ALL] Filling count and displs arrays
    SetScatterArrays(num_elem, diag, mtx, num_workers, row, col, row_counts, row_displs,
                     col_counts, col_displs);
    int local_count = row_counts[my_rank];

    std::cout << "Worker " << my_rank << " with "
              << "fst_count = " << row_counts[my_rank] << " fst_displs = " << row_displs[my_rank]
              << "snd_count = " << col_counts[my_rank] << " snd_displs = " << col_displs[my_rank] << std::endl;

    // [ALL] Calling Scatterv for the row vector
    MPI_Scatterv(mtx.data.data(),
                 row_counts.data(),
                 row_displs.data(),
                 MPI_DOUBLE,
                 local_row.data(),
                 local_count,
                 MPI_DOUBLE,
                 MASTER_RANK,
                 MPI_COMM_WORLD);

    // [ALL] Calling Scatterv for the col vector
    MPI_Scatterv(mtx.data.data(),
                 col_counts.data(),
                 col_displs.data(),
                 MPI_DOUBLE,
                 local_col.data(),
                 local_count,
                 MPI_DOUBLE,
                 MASTER_RANK,
                 MPI_COMM_WORLD);

    // [ALL] Executing local DotProd
    temp = 0.0;
    for(u64 i = 0; i < col_counts[my_rank]; ++i)
        temp += local_row[i] + local_col[i];

    // TODO [ALL] Calling Reduce
    MPI_Reduce(&temp,
               &total_sum,
               1,
               MPI_DOUBLE,
               MPI_SUM,
               MASTER_RANK,
               MPI_COMM_WORLD);

    if(my_rank == MASTER_RANK) {
        // [MASTER] Applying cube_root to result
        std::cout << "Received total value" << total_sum << std::endl;
        total_sum = std::cbrt(total_sum);

        // [MASTER] Storing result in the matrix
        mtx.SetValue(row, col, total_sum);
    }

}

// TODO DESCRIPTION
inline void MPIWavefront(const int& my_rank, const int num_workers, DiagInfo& diag, SquareMtx& mtx,
                  vec_int& row_counts, vec_int& row_displs,
                  vec_int& col_counts, vec_int& col_displs,
                  vec_double& local_row, vec_double& local_col,
                  double& temp, double& total_sum) {

    while(diag.num < mtx.length) {
        if(my_rank == MASTER_RANK)
            std::cout << "Computing diagonal " << diag.num << std::endl;
        for(u64 elem = 1; elem <= diag.length; ++elem) {
            if(my_rank == MASTER_RANK)
                std::cout << "Computing elem " << elem << std::endl;
            ComputeElem(my_rank, elem, diag, mtx, num_workers,
                       row_counts, row_displs,
                       col_counts, col_displs,
                       local_row, local_col,
                       temp, total_sum);
        }

        diag.PrepareNextDiagonal();
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
    // Setting Base Parameters
    u64 mtx_length{default_length};
    if (argc >= 2)      // If the user as passed its own lenght
                        // for the matrix use it instead
            mtx_length = std::stoul(argv[1]);

    // Initialize MPI and Setting MPI parameters
    if ( MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        std::cerr<<"in MPI_Init"<<std::endl;
        return EXIT_FAILURE;
    }

    // Setting up common params
    int my_rank{0};  int num_processes{0};
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    DiagInfo diag(mtx_length, num_processes);
    SquareMtx mtx; // BE AWARE: Only the Master will initialize it!
    mtx.length = mtx_length;
    vec_int row_counts(num_processes);
    vec_int row_displs(num_processes);
    vec_int col_counts(num_processes);
    vec_int col_displs(num_processes);
    vec_double local_row;
    vec_double local_col;
    double temp{0.0};
    double total_sum{0.0};

    if (my_rank == MASTER_RANK)
        std::cout << "Starting WaveFront Computation with:\n"
                  << "num_processes = " << num_processes << "\n"
                  << "mtx.length = " << mtx_length  << std::endl;

    MPIWavefront(my_rank, num_processes, diag, mtx,
                 row_counts, row_displs, col_counts, col_displs,
                 local_row, local_col, temp, total_sum);

    // Exiting the application
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    mtx.PrintMtx();
    return EXIT_SUCCESS;
}