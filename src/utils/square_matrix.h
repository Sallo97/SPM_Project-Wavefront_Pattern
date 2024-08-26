/**
 * @file square_matrix.h
 * @brief The Square matrix class used throught the project.
 * @details This class represents a std::vector<double> interpreted as a Matrix, offering
 *          a set of methods which facilitate the use for the user by abstracting it
 *          as a real matrix.
 * @author Salvatore Salerno
 */

#ifndef SQUARE_MATRIX_H
#define SQUARE_MATRIX_H

#include "constants.h"

// Struct for our Square Matrix
// The matrix is initialized as a matrix of size nxn s.t.
// ∀m in [0, n].mtx[m][m] = (m + 1) / n.
// All other elements in the matrix are set to 0.
/**
 * @brief Represents a Square Matrix, storing all the data in a single std::vector<double>.
 */
struct SquareMtx {

    /**
     * @brief Creates an initialized matrix
     * @param[in] length = sets the lenght of the matrix.
     */
    explicit SquareMtx(const u64 &length) : length(length), data(length*length, 0) { InitializeMatrix(); }

    /**
     * @brief Return the real index where the cell mtx[row][col]
     *        is stored.
     * @param[in] row = the row index of the cell.
     * @param[in] col = the column index of the cell.
     */
    [[nodiscard]] u64 GetIndex(const u64 row, const u64 col) const { return (row * length) + col; }

    /**
     * @brief Initialize the matrix with the length it has s.t.
     * ∀m in [0, n].mtx[m][m] = (m + 1) / n.
     * All other elements in the matrix are set to 0.
     */
    void InitializeMatrix() {
        for (u64 m = 0; m < length; ++m)
            data[(m * length) + m] = static_cast<double>(m + 1) / static_cast<double>(length);
    }

    /**
     * @brief Return the value at position mtx[row][col]
     * @param[in] row = the row index of the cell.
     * @param[in] col = the column index of the cell.
     */
    [[nodiscard]] double GetValue(const u64 row, const u64 col) const {
        if (length == 0) {
            std::cerr << "ERROR!!!! Matrix not initialized" << std::endl;
            return -1.0;
        }
        return data[GetIndex(row, col)];
    }

    /**
     * @brief Does mtx[row][col] = val
     * @param[in] row = the row index of the cell.
     * @param[in] col = the column index of the cell.
     * @param[in] val = the value to set in mtx[row][col]
     */
    void SetValue(u64 row, const u64 col, const double val) {
        if (length == 0) {
            std::cerr << "ERROR!!!! Matrix not initialized" << std::endl;
            return;
        }
      data[GetIndex(row, col)] = val;
      data[GetIndex(col, row)] = val;
    }

    /**
     * @brief Prints the whole matrix
     */
    void PrintMtx() const {
        if (length == 0) {
            std::cerr << "ERROR!!!! Matrix not initialized" << std::endl;
            return;
        }
        if (length > 100) {
            std::cerr << "ERROR!!! The matrix is too big to print" << std::endl;
            return;
        }

        std::cout << "\n";
        for (u64 i = 0; i < length; ++i) {
            for (u64 j = 0; j < length; ++j)
                std::cout << data[GetIndex(i, j)] << " ";
            std::cout << "\n";
        }
    }

    /**
     *  @brief Checks if the element mtx[row][col] has already been done (it is != 0)
     *  @param[in] row = row of the elem
     *  @param[in] col = col of the elem
    */
    [[nodiscard]] bool IsElemAlreadyDone(const u64 row, const u64 col) const {
      return (data[GetIndex(row,col)] != 0);
    }

    // Parameters
    u64 length{0};
    std::vector<double> data;
};
#endif // SQUARE_MATRIX_H
