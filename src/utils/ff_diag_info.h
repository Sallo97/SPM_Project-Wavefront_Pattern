//
// Created by Salvatore Salerno on 11/08/24. TODO
//

#ifndef FF_SEND_INFO_H
#define FF_SEND_INFO_H

#include "constants.h"


/**
 * @brief Contains informations for the Emitter regarding
 *        when and what diagonal to send to the Workers
 */
struct SendInfo {
    explicit SendInfo(const u64 base_lenght, const u64 num_workers)
    : diag_length(base_lenght), num_workers(num_workers){PrepareNextDiagonal();}

    /**
    * @brief This method prepare all parameters of the Emitter for computing
    *        the elements of the next diagonal of the matrix.
    */
    void PrepareNextDiagonal() {
        send_tasks = true;
        diag++;
        diag_length--;
        active_workers = 0;
        chunk_size = static_cast<u64>(std::ceil(diag_length / num_workers));
    }

    // PARAMETERS
    u8 num_workers;
    u64 chunk_size{0};
    u64 diag{0};                // The current diagonal.
    u64 diag_length{0};         // Number of elements of the curr. diagonal.
};

#endif //FF_SEND_INFO_H
