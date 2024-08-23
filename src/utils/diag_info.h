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
    explicit DiagInfo(const u64 base_length, const int num_workers) :
        num_actors(num_workers), length(base_length)
#ifdef DYNAMIC_CHUNK
        , id_chunks{static_cast<u64>(num_actors), 0}
#endif
    {
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
#ifdef DYNAMIC_CHUNK
        AddDynamicChunk(0, ff_chunk_size);
#endif
    }

#ifdef DYNAMIC_CHUNK
    /**
     * @brief Adds the dynamic chunk_size of id_chunk in the associated array
     * @param[in] id_chunk = the id_chunk, consider they start at position 1
     * @param[in] val = the chunk_size to store
     */
    void AddDynamicChunk(const int id_chunk, const u64 val) { id_chunks[id_chunk - 1] = val; }

    /**
     * @brief Adds the dynamic chunk_size of id_chunk in the associated array
     * @param[in] id_chunk = the id_chunk, consider they start at position 1.
     */
    [[nodiscard]] u64 GetDynamicChunkSize(const int id_chunk) const { return id_chunks[id_chunk - 1]; }
#endif

    /**
     * @brief Sets the chunk size used in the FastFlow case.
     *        chunk_size = upper_integer_part(diagonal_length / num_workers)
     */
    void ComputeFFChunkSize() {
        ff_chunk_size = std::ceil((static_cast<double>(length) / static_cast<double>(num_actors)));
        if (ff_chunk_size == 0)
            ff_chunk_size = 1;
    }

    // PARAMETERS
    const int num_actors;
    u64 length{0}; // Number of elements of the curr. diagonal.
#ifdef DYNAMIC_CHUNK
    std::vector<u64> id_chunks; // Contains the dynamic chunk for each id_chunk
#endif
    u64 ff_chunk_size{0};
    u64 num{0}; // Number of the current diagonal. The major diagonal is counted as 0.
};

#endif // DIAG_INFO_H
