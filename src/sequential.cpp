/**
 * @file sequential.cpp
 * @brief This code is a sequential implementation of the Wavefront computation.
 * @details It initialize a matrix of dimension NxN (parameter given as input).
 *          At the beginning the values are all set to 0 except
 *          the ones in the major diagonal s.t.:
 *          ∀i in [0, N].mtx[i][i] = (i + 1) / n.
 *          Then all elements in the upper part of the major diagonal are computed s.t.:
 *          mtx[i][j] = square_cube( dot_product(v_m, v_m+k) )
 * @author Salvatore Salerno
 */

#include <cmath>
#include <iostream>
#include <vector>
#include "utils/square_matrix.h"
#include "utils/elem_info.h"

using u64 = std::uint64_t;
constexpr u64 default_length = 1 << 14;  // it is read as "2^14"

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
inline void ComputeElement(SquareMtx& mtx, ElemInfo& elem,
                       u64& vec_length, double& res) {
    res = 0.0; // Reset the result value
    const ElemInfo fst_elem_vec_row{elem.GetVecRowElem()}; // Indexes for the first vector

    const ElemInfo fst_elem_vec_col{elem.GetVecColElem()}; // Indexes for the second vector
                                                           // In reality We do not work with the column vector
                                                           // but with a row in the lower triangular
                                                           // that contains the same elements

    // Starting the DotProduct Computation
    for(u64 i = 0; i < vec_length; ++i)
        res += mtx.GetValue(fst_elem_vec_row.row, fst_elem_vec_row.col + i)
                * mtx.GetValue(fst_elem_vec_col.row, fst_elem_vec_col.col + i);

    // Storing the cuberoot of the final result
    res = std::cbrt(res);
}

/**
 * @brief Apply the WaveFront Computation.
 *        For all elements in diagonal above the major diagonal
 *        it computes them s.t. mtx[i][j] =
 *                                     cube_root(DotProduct(v_m, v_(m+k))
 * @param[out] mtx = the reference to the matrix to initialize.
 */
inline void ComputeMatrix(SquareMtx& mtx) {
    double temp{0.0}; // Stores the results of
                      // the DotProduct computations.
    u64 diag_length{mtx.length};    // Stores the number of elements
                                        // in each diagonal

    for (u64 num_diag = 1; num_diag < mtx.length; ++num_diag) { // We assume the major diagonal has number 0
        diag_length--;

        for(u64 i = 1; i <= diag_length; ++i) {
            ElemInfo curr_elem{mtx.length, num_diag, i};

            ComputeElement(mtx, curr_elem, num_diag, temp);

            // Storing the result
            mtx.SetValue(curr_elem, temp);
            mtx.SetValue(curr_elem.col, curr_elem.row, temp);
        }
    }
}

/**
 * @brief A Sequential Wavefront Computation
 * @param[in] argv[1] is the lenght of a row (or a column) of the matrix
 * @param[in] argc = the number of cmd arguments.
 * @note If no argument is passed, then we assume the matrix has default length
 */
int main(const int argc, char *argv[]) {
    // Setting matrix length
    u64 mtx_length = default_length;
    if (argc >= 2) // If the user as passed its own lenght
                   // for the matrix use it instead
        mtx_length = std::stoull(argv[1]);

    // Initialize Matrix
    SquareMtx mtx(mtx_length);

    // Beginning WaveFront Pattern
    const auto start = std::chrono::steady_clock::now();

    if (mtx_length > 1) // If we have a 1x1 matrix, then we
                        // do not need to apply the computation
        ComputeMatrix(mtx);

    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Time taken for sequential version: " << duration.count() << " milliseconds" << std::endl;
    return 0;
}