/**
* @file compute_elem.h
 * @brief This code contains the common function ComputeElement which computes
 *        the element in a WaveFront Computation. It is used by both the sequential
 *        and FastFlow implementation, so for better reusability it has been putted here.
 * @author Salvatore Salerno
 */

#ifndef COMPUTE_ELEM_H
#define COMPUTE_ELEM_H

#include "cmath"
#include "square_matrix.h"
#include "constants.h"

#if defined(_OPENMP)
#include <omp.h>
#endif

/**
 * @brief Compute for an element mtx[i][j] the DotProduct of the vectors
 *        mtx[i][j] = cube_root( DotProd(row[i],col[j]) ).
 *        The dot product is done on elements already computed
 * @param[in] mtx = the reference to the matrix.
 * @param[in] elem_row = the row where the element to compute is
 * @param[in] elem_col = the col. where the element to compute is
 * @param[in] vec_length = the length of the vectors of the DotProduct
 *                         (usually is equal to the diagonal where the elem is from).
 * @param[out] res = where to store the result
*/
inline void ComputeElement(const SquareMtx & mtx, const u64& elem_row, const u64& elem_col,
                           const u64& vec_length, double& res) {
    // Reset the result value
    res = 0.0;

    // // Determining row and cols of the mtx
    // const u64& fst_vec_row = elem_row;   const u64& fst_vec_col = elem_row;
    // const u64& snd_vec_row = elem_col;   const u64 snd_vec_col = elem_row + 1;

    // In reality We do not work with the column vector
    // but with a row in the lower triangular
    // that contains the same elements

#pragma omp parallel for reduction(+:res)
    // Starting the DotProduct Computation
    for(u64 i = 0; i < vec_length; ++i)
        res += mtx.GetValue(elem_row, elem_row + i)
                * mtx.GetValue(elem_col, elem_row + 1 + i);

    // Storing the cuberoot of the final result
    res = std::cbrt(res);
}

#endif //COMPUTE_ELEM_H
