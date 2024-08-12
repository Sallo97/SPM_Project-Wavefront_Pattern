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
    SendInfo(const u64 num_workers, const u64 base_lenght)
    : diag_length(base_lenght), num_workers(num_workers) {PrepareNextDiagonal();}

    /**
    * @brief This method prepare all parameters of the Emitter for computing
    *        the elements of the next diagonal of the matrix.
    */
    void PrepareNextDiagonal() {
        send_tasks = true;
        diag++;
        diag_length--;
        computed_elements = diag_length;
    }

    // PARAMETERS
    u64 diag{0};                // The current diagonal.
    u64 diag_length{0};         // Number of elements of the curr. diagonal.
    u64 num_workers{0};         // Number of elements per Worker.
    u64 computed_elements{0};   // Number of computed elements during a cycle
    bool send_tasks{false};     // Tells if we have to send tasks to Workers.
};

#endif //FF_SEND_INFO_H
