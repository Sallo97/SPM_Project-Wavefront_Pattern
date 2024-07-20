/**
 * @file sequential.cpp
 * @brief This code is a sequential implementation of the Wavefront computation.
 * @details Sequential implementation of the Wavefront computation.
 *          It initialize a matrix of dimension NxN (parameter given as input).
 *          All values are set to 0 except the one in the major diagonal s.t.:
 *          ∀i in [0, N].mtx[i][i] = (i + 1) / n.
 *          Then all elements in the upper part of the major diagonal are computed s.t.:
 *          mtx[i][j] = square_cube( dot_product(v_m, v_m+k) )
 * @author Salvatore Salerno
 */

#include <cmath>
#include <iostream>
#include <vector>


// Struct for our Square Matrix
// The matrix is initialized as a matrix of size nxn s.t.
// ∀m in [0, n].mtx[m][m] = (m + 1) / n.
// All other elements in the matrix are set to 0.
struct SquareMtx {
    //Constructor
    explicit SquareMtx(int row_length)
        :data(row_length*row_length, 0), row_length(row_length)
    {
        for(int m{0}; m < row_length; ++m)
            data[ (m * row_length) + m ] = static_cast<double>(m + 1)/ static_cast<double>(row_length) ;
    }

    /**
        * @brief Return the real index where the cell mtx[row][col]
        *        is stored.
        * @param[in] row = the row index of the cell.
        * @param[in] col = the column index of the cell.
    */
    [[nodiscard]] int GetIndex(int row, int col) const {
        return (row * row_length) + col;
    }

    /**
        * @brief Return the value at position mtx[row][col]
        * @param[in] row = the row index of the cell.
        * @param[in] col = the column index of the cell.
    */
    double GetValue(int row, int col) {
        return data[GetIndex(row, col)];
    }

    /**
        * @brief Does mtx[row][col] = val
        * @param[in] row = the row index of the cell.
        * @param[in] col = the column index of the cell.
        * @param[in] val = the value to set in mtx[row][col]
    */
    void SetValue(int row, int col, double val) {
        data[GetIndex(row, col)] = val;
    }
    /**
        * @brief Prints the matrix
    */
    void PrintMtx(){
        std::cout << "\n";
        for(int i = 0; i < row_length; ++i) {
            for(int j = 0; j < row_length; ++j)
                std::cout << data[GetIndex(i, j)] << " ";
            std::cout << "\n";
        }
    }

    // Parameters
    std::vector<double> data;
    int row_length;
};

/**
 * @brief Apply the WaveFront Computation.
 *        For all elements in diagonal above the major diagonal
 *        it computes them s.t. mtx[i][j] =
 *                                     cube_root(DotProduct(v_m, v_(m+k))
 * @param[in] n = the length of a row (or a column) of the matrix.
 * @param[out] mtx = the reference to the matrix to initialize.
 */
void ComputeMatrix(const int n, SquareMtx& mtx) {
    double temp{0.0}; // Temporary value used to store
                      // the DotProduct computations.

    for (int iter{0}; iter < n; ++iter) { // iter = number of the current diagonal
                                          // Setting parameters for current iteration
        int i = 0;
        int j = iter + 1;

        // Computing the elements of the current diagonal
        while (j < n) {
            // Setting parameters for DotProduct
            const int idx_vecr_row = i; int idx_vecr_col = i; // Indexes for the first vector
            const int idx_vecc_row = j; int idx_vecc_col = i + 1; // Indexes for the second vector
                                                                  // We do not work with the column vector
                                                                  // but with a row in the lower triangular
                                                                  // that contains the same elements

            // Computing the DotProduct
            for (int elem{0}; elem < j - i; ++elem) {
                temp += mtx.GetValue(idx_vecr_row, idx_vecr_col + elem) * mtx.GetValue(idx_vecc_row, idx_vecc_col + elem);
            }
            // Applying the cube root
            mtx.SetValue(i, j, std::cbrt(temp));
            mtx.SetValue(j, i, std::cbrt(temp));
            temp = 0.0;
            ++i;
            ++j;
        }
    }
}

/**
 * @brief A Sequential Wavefront Computation
 * @param[in] argv[1] is the lenght a row (or a column) of the matrix
 * @exception argc < 2
 */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "You must pass as argument the lenght of the square matrix to generate" << std::endl;
        return -1;
    }

    if (std::stoll(argv[1]) <= 0) {
        std::cerr << "Invalid lenght! The lenght must be a positive integer != 0" << std::endl;
        return -1;
    }
    const auto n{static_cast<int>(std::stoll(argv[1]))};


    // Initialize mtx
    SquareMtx mtx(n);

    // Beginning WaveFront Pattern
    const auto start = std::chrono::steady_clock::now();

    if (n > 1) // If we have a 1x1 matrix, then we
               // do not need to apply the computation
        ComputeMatrix(n, mtx);

    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken for sequential version: " << duration.count() << " milliseconds" << std::endl;
    return 0;
}