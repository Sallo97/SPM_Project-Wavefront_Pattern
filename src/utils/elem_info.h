//
// Created by Salvatore Salerno on 25/07/24.
//

#ifndef ELEM_INFO_H
#define ELEM_INFO_H

using uint64 = std::uint64_t;

struct ElemInfo {
  /**
    * @brief Base constructor
    * @param[in] row = the row of the element
    * @param[in] col = the col. of the element
 */
  ElemInfo(uint64 row = 0, uint64 col = 0)
    :row(row), col(col){ }

  /**
    * @brief Construct the element of a diagonal.
    * @param[in] mtx_length = the length of the row of the matrix
    * @param[in] num_diag = the diagonal of the matrix (we assume that
    *                       the major diagonal has num 0)
    * @param[in] num_elem = the position of the element in the diagonal
    *                       (we assume the first element has position 1)
  */
  ElemInfo(uint64 mtx_length, uint64 num_diag, uint64 num_elem) {
    if(mtx_length <= num_diag) {
      std::cerr << "ERROR!!! The element is not in an upper diagonal" << std::endl;
      return;
    }
    row = num_elem - 1;
    col = row + num_diag;
  }

  /**
  * @brief Returns an ElemInfo obj. relative to the first element
  *        of the row vector for the DotProduct of the element
  *        associated in the current obj.
  */
  ElemInfo GetVecRowElem() {
    if(row > col) {
      std::cerr << "ERROR!!! The current element is not in an upper diagonal" << std::endl;
    }

    return ElemInfo{row, row};
  }

  /**
  * @brief Returns an ElemInfo obj. relative to the first element
  *        of the row vector for the DotProduct of the element
  *        associated in the current obj.
  *        WARNING: In reality we do not work with the column vector
  *                 but with a row in the lower triangular that
  *                 contains the same elements
  */
  ElemInfo GetVecColElem() {
    if(row > col) {
      std::cerr << "ERROR!!! The current element is not in an upper diagonal" << std::endl;
    }
    return ElemInfo{col, row + 1};
  }
  //PARAMETERS
  uint64 row{0};
  uint64 col{0};
};
#endif //ELEM_INFO_H
