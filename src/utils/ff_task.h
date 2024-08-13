//
// Created by Salvatore Salerno on 11/08/24. TODO ADD A DESCRIPTION
//

#ifndef FF_TASK_H
#define FF_TASK_H

/**
 * @brief Represent a task that the Emitter will give to one of
 *        its Workers.
 */
#include "constants.h"

struct Task{

    // PARAMETERS
    u64 start_range;           // First element of the range of elements to compute
    u64 end_range;            // Last element of the range of elements to compute
};


#endif //FF_TASK_H
