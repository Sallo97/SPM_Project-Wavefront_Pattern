//
// Created by Salvatore Salerno on 11/08/24. TODO
//

#ifndef FF_DIAG_INFO_H
#define FF_DIAG_INFO_H

#include "constants.h"


/**
 * @brief Contains informations about the current diagonal to compute
 *        when and what diagonal to send to the Workers.
 *        ONLY THE EMITTER MODIFIES IT, THE WORKER ONLY READ IT
 */
struct DiagInfo {
    explicit DiagInfo(const u64 base_lenght, const u64 num_workers)
    : num_workers(num_workers), length(base_lenght) {PrepareNextDiagonal();}

    /**
    * @brief This method prepare all parameters of the Emitter for computing
    *        the elements of the next diagonal of the matrix.
    */
    void PrepareNextDiagonal() {
        num++;
        length--;
        chunk_size = static_cast<u64>(std::ceil(length / num_workers));
        if (chunk_size == 0)
            chunk_size = 1;
    }

    // PARAMETERS
    const u8 num_workers;
    u64 chunk_size{0};
    u64 num{0};                // The current diagonal.
    u64 length{0};         // Number of elements of the curr. diagonal.
};

#endif //FF_DIAG_INFO_H
