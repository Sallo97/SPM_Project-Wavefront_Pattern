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

#include<vector>
#include<iostream>
#include<cmath>

/**
 * @brief Apply the WaveFront Computation.
 *        For all elements in diagonal above the major diagonal
 *        it computes them s.t. mtx[i][j] =
 *                                     cube_root(DotProduct(v_m, v_(m+k))
 * @param[in] n = the length of a row (or a column) of the matrix.
 * @param[out] mtx = the reference to the matrix to initialize.
 */
void ComputeMatrix(const uint64_t n, std::vector<double>& mtx) {
 double temp{0.0}; // Temporary value used to store
                   // the DotProduct computation.

 for(uint64_t iter{0}; iter < n; ++iter) {
  // Setting parameters for current iteration
  uint64_t i = 0; uint64_t j = iter + 1;

  // Computing the elements of the current diagonal
  while( j < n ) {
   // Setting parameters for DotProduct
   const uint64_t idx_vecr_row = i;   uint64_t idx_vecr_col = i;     // Indexes for the first vector
   const uint64_t idx_vecc_row = j;  uint64_t idx_vecc_col = i + 1;  // Indexes for the second vector
                                                                     // We do not work with the column vector
                                                                     // but with a row in the lower triangular
                                                                     // that contains the same elements

   // Computing the DotProduct
   for(uint64_t elem_vec{0}; elem_vec < j-i ; ++elem_vec) {
    temp += mtx[idx_vecr_row * n + idx_vecr_col] *
                      mtx[idx_vecc_row * n + idx_vecc_col];
    idx_vecr_col++;
    idx_vecc_col++;
   }
   ++i; ++j;
  }

  // Applying the cube root
  mtx[i * n + j] = std::cbrt(temp);
  mtx[j * n + i] = temp;  // storing the element in the lower triangular
  temp = 0.0;
 }
}

/**
 * @brief A Sequential Wavefront Computation
 * @param[in] argv[1] is the lenght a row (or a column) of the matrix
 * @exception argc < 2
 */
int main(int argc, char* argv[]) {
 if(argc!=2) {
  std::cerr << "You must pass as argument the lenght of the square matrix to generate"
            << std::endl;
  return -1;
 }

 if (std::stoll(argv[1]) <= 0) {
  std::cerr << "Invalid lenght! The lenght must be a positive integer != 0"
            << std::endl;
  return -1;
 }
 const auto n{static_cast<int>(std::stoll(argv[1]))};



 // Initialize mtx as a matrix of size nxn s.t.
 // ∀m in [0, n].mtx[m][m] = (m + 1) / n.
 // All other elements in the matrix are set to 0.
 std::vector<double> mtx{static_cast<double>(n*n), 0};
 for(int m{0}; m < n; ++m)
     mtx[ (m * n) + m ] = static_cast<double>(m + 1)/ static_cast<double>(n) ;

 // Beginning WaveFront Pattern
 const auto start = std::chrono::steady_clock::now();

 if(n > 1) // If we have a 1x1 matrix, then we
           // do not need to apply the computation
   ComputeMatrix(n, mtx);

 const auto end = std::chrono::steady_clock::now();
 const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
 std::cout << "Time taken for sequential version: " << duration.count() << " milliseconds" << std::endl;

 return 0;
}