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

#include <iostream>
#include <vector>
#include "utils/square_matrix.h"
#include "utils/elem_info.h"
#include "utils/constants.h"
#include "utils/compute_elem.h"

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
    u16 diag_length{mtx.length};    // Stores the number of elements
                                        // in each diagonal

    for (u16 num_diag = 1; num_diag < mtx.length; ++num_diag) { // We assume the major diagonal has number 0
        diag_length--;

        for(u16 i = 1; i <= diag_length; ++i) {
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
    u16 mtx_length = default_length;
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
    mtx.PrintMtx();
    return 0;
}