/**
* @file square_matrix.h
 * @brief The Square matrix class used throught the project.
 * @details TODO ADD A DESCRIPTION
 * @author Salvatore Salerno
 */

#ifndef SQUARE_MATRIX_H
#define SQUARE_MATRIX_H

#include "elem_info.h"
#include "constants.h"

// Struct for our Square Matrix
// The matrix is initialized as a matrix of size nxn s.t.
// ∀m in [0, n].mtx[m][m] = (m + 1) / n.
// All other elements in the matrix are set to 0.
struct SquareMtx {
    //Constructor
    SquareMtx()= default;

    explicit SquareMtx(const u64& length)
        :length(length) {
        InitializeMatrix();
    }

    /**
        * @brief allocated the data buffer with length elements
    */
    void AllocateData() {
        if(length == 0) {
            std::cerr << "ERROR!!!! Matrix not initialized" << std::endl;
        }
        data = new std::vector<double>(length, 0.0);
    }

    /**
        * @brief Return the real index where the cell mtx[row][col]
        *        is stored.
        * @param[in] row = the row index of the cell.
        * @param[in] col = the column index of the cell.
    */
    [[nodiscard]] u64 GetIndex(const u64 row, const u64 col) const {
        return (row * length) + col;
    }

    /**
        * @brief Return the real index where the cell mtx[row][col]
        *        is stored.
        * @param[in] elem = an ElemInfo object.
    */
    [[nodiscard]] u64 GetIndex(const ElemInfo elem) const {
        return (elem.row * length) + elem.col;
    }

    /**
        * @brief Initialize the matrix with the length it has.
    */
    void InitializeMatrix() {
        AllocateData();
        for(u64 m{0}; m < length; ++m)
            (*data)[ (m * length) + m ] = static_cast<double>(m + 1)/ static_cast<double>(length) ;
    }

    /**
    * @brief Initialize the matrix with the length it has.
*/
    void InitializeMatrix(u64 length) {
        AllocateData();
        for(u64 m{0}; m < length; ++m)
            (*data)[ (m * length) + m ] = static_cast<double>(m + 1)/ static_cast<double>(length) ;
    }

    /**
        * @brief Return the value at position mtx[row][col]
        * @param[in] row = the row index of the cell.
        * @param[in] col = the column index of the cell.
    */
    [[nodiscard]] double GetValue(const u64 row, const u64 col) const {
        if(length == 0) {
            std::cerr << "ERROR!!!! Matrix not initialized" << std::endl;
            return -1.0;
        }
        return (*data)[GetIndex(row, col)];
    }

    /**
        * @brief Does mtx[row][col] = val
        * @param[in] row = the row index of the cell.
        * @param[in] col = the column index of the cell.
        * @param[in] val = the value to set in mtx[row][col]
    */
    void SetValue(u64 row, const u64 col, const double val) {
        if(length == 0) {
            std::cerr << "ERROR!!!! Matrix not initialized" << std::endl;
            return;
        }
        (*data)[GetIndex(row, col)] = val;
    }

    /**
    * @brief Does mtx[row][col] = val. The main difference is that
    *        here we pass a ElemInfo obj
    * @param[in] elem = reference containing informations
    *                   regarding the element
    * @param[in] val = the value to store
    */
    void SetValue(const ElemInfo& elem, const double val) {
        if(length == 0) {
            std::cerr << "ERROR!!!! Matrix not initialized" << std::endl;
            return;
        }
        (*data)[GetIndex(elem.row, elem.col)] = val;
    }

    /**
        * @brief Prints the whole matrix
    */
    void PrintMtx() const{
        if(length == 0) {
            std::cerr << "ERROR!!!! Matrix not initialized" << std::endl;
            return;
        }
        if (length > 100) {
            std::cerr << "ERROR!!! The matrix is too big to print" << std::endl;
            return;
        }

        std::cout << "\n";
        for(u64 i = 0; i < length; ++i) {
            for(u64 j = 0; j < length; ++j)
                std::cout << (*data)[GetIndex(i, j)] << " ";
            std::cout << "\n";
        }
    }

    /**
        * @brief DEBUG function, fills the matrix with
        *        s.t. ∀ mtx[i][j] = j
    */
    void FillMatrix() {
        if(length == 0) {
            std::cerr << "ERROR!!!! Matrix not initialized" << std::endl;
            return;
        }

        for(u64 i = 0; i < length; ++i)
            for (u64 j = 0; j < length; ++j)
                SetValue(i, j, static_cast<double>(j));
    }

    // Parameters
    std::vector<double>* data{nullptr};
    u64 length{0};
};
#endif //SQUARE_MATRIX_H
