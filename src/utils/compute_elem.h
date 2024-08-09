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

/**
 * @brief Compute for an element mtx[i][j] the DotProduct of the vectors
 *        mtx[i][j] = cube_root( DotProd(row[i],col[j]) ).
 *        The dot product is done on elements already computed
 * @param[in] mtx = the reference to the matrix.
 * @param[in] elem = contains information regarding the element
 *                   that we need to compute.
 * @param[in] vec_length = the length of the vectors of the DotProduct
 *                         (usually is equal to the diagonal where the elem is from).
 * @param[out] res = where to store the result
*/
inline void ComputeElement(const SquareMtx & mtx, ElemInfo& elem,
                           const u16 & vec_length, double& res) {
    res = 0.0; // Reset the result value
    const ElemInfo fst_elem_vec_row{elem.GetVecRowElem()}; // Indexes for the first vector

    const ElemInfo fst_elem_vec_col{elem.GetVecColElem()}; // Indexes for the second vector
    // In reality We do not work with the column vector
    // but with a row in the lower triangular
    // that contains the same elements

    // Starting the DotProduct Computation
    for(u16 i = 0; i < vec_length; ++i)
        res += mtx.GetValue(fst_elem_vec_row.row, fst_elem_vec_row.col + i)
                * mtx.GetValue(fst_elem_vec_col.row, fst_elem_vec_col.col + i);

    // Storing the cuberoot of the final result
    res = std::cbrt(res);
}

#endif //COMPUTE_ELEM_H
