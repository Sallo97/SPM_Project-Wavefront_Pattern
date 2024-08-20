//
// Created by Salvatore Salerno on 19/08/24.
//

#ifndef COMPUTE_RANGE_H
#define COMPUTE_RANGE_H

#include "compute_elem.h"


/**
 * @brief Computes the elements in the given range
 * @param start_range = first element of the diagonal
 * @param end_range = last elem of the diagonal
 * @param num_diag = number of the diagonal
 * @param mtx = obj storing the matrix
 */
inline void ComputeRange(u64 start_range, const u64 end_range, const u64 length, const u64 num_diag, SquareMtx& mtx) {
    // If start_range is out of bounds skip computation
    if (start_range > length)
        return;

    // Starting Computation
    double temp = 0.0;
    while(start_range <= end_range){

        // Determining element
        // ElemInfo curr_elem{mtx.length, num_diag, start_range};
        u64 row = start_range - 1;
        u64 col = row + num_diag;

        // Computing element
        if(!mtx.IsElemAlreadyDone(row, col)) {
            ComputeElement(mtx, row, col, num_diag, temp);
            // Storing the result
            mtx.SetValue(row, col, temp);
        }

        // Setting next element to compute
        ++start_range;
    }
}

#endif //COMPUTE_RANGE_H
