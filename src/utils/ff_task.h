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
    u16 start_range;           // First element of the range of elements to compute
    u16 end_range;            // Last element of the range of elements to compute
    u16 diag;                 // The upper diagonal where the elements are
                              // We assume that the major diagonal has value 0
};


#endif //FF_TASK_H
