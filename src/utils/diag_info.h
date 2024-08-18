/**
 * @file diag_info.h
 * @brief The DiagInfo class used in the FastFlow and MPI Implementation.
 *        It contains informations regarding the current diagonal of the
 *        matrix being computed.
 * @author Salvatore Salerno
 */


#ifndef DIAG_INFO_H
#define DIAG_INFO_H

#include "constants.h"

/**
 * @brief Contains informations about the current diagonal to compute
 *        when and what diagonal to send to the Workers.
 *        ONLY THE EMITTER MODIFIES IT, THE WORKER ONLY READ IT
 */
struct DiagInfo {
    /**
     * @brief Constructor of the class
     * @param[in] base_length = the length of the matrix.
     * @param[in] num_workers = the number of Workers in execution.
     */
    explicit DiagInfo(const u64 base_length, const int num_workers) : num_actors(num_workers), length(base_length) {
        PrepareNextDiagonal();
    }

    /**
     * @brief This method prepare all parameters of the Emitter for computing
     *        the elements of the next diagonal of the matrix.
     */
    void PrepareNextDiagonal() {
        num++;
        length--;
        ComputeFFChunkSize();
        ComputeMPIChunkSize();
    }

    /**
     * @brief Sets the chunk size used in the FastFlow case.
     *        chunk_size = upper_integer_part(diagonal_length / num_workers)
     */
    void ComputeFFChunkSize() {
        ff_chunk_size = std::ceil((static_cast<double>(length) / static_cast<double>(num_actors)));
        if (ff_chunk_size == 0)
            ff_chunk_size = 1;
    }

    /**
     * @brief Sets the chunk size used in the MPI case.
     *        chunk_size = upper_integer_part(row_or_col_vector / num_workers)
     *        Note: the length of a vector of the DotProduct equals to the number of
     *              the current diagonal
     */
    void ComputeMPIChunkSize() {
        mpi_chunk_size = std::ceil((static_cast<double>(num) / static_cast<double>(num_actors)));
        if (mpi_chunk_size == 0)
            mpi_chunk_size = 1;
    }

    // PARAMETERS
    const int num_actors;
    u64 ff_chunk_size{0};
    u64 mpi_chunk_size{0};
    u64 num{0};    // Number of the current diagonal. The major diagonal is counted as 0.
    u64 length{0}; // Number of elements of the curr. diagonal.
};

#endif // DIAG_INFO_H
