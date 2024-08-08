/**
* @file square_matrix.h
 * @brief The Square matrix class used throught the project.
 * @details TODO ADD A DESCRIPTION
 * @author Salvatore Salerno
 */

#ifndef SQUARE_MATRIX_H
#define SQUARE_MATRIX_H

#include "elem_info.h"

using uint64= std::uint64_t;

// Struct for our Square Matrix
// The matrix is initialized as a matrix of size nxn s.t.
// ∀m in [0, n].mtx[m][m] = (m + 1) / n.
// All other elements in the matrix are set to 0.
struct SquareMtx {
    //Constructor
    SquareMtx(){row_length = -1;}

    explicit SquareMtx(uint64 row_length)
        :data(row_length*row_length, 0), row_length(row_length) {
        InitializeMatrix();
    }

    /**
        * @brief Return the real index where the cell mtx[row][col]
        *        is stored.
        * @param[in] row = the row index of the cell.
        * @param[in] col = the column index of the cell.
    */
    [[nodiscard]] uint64 GetIndex(uint64 row, uint64 col) const {
        return (row * row_length) + col;
    }

    /**
        * @brief Return the real index where the cell mtx[row][col]
        *        is stored.
        * @param[in] elem = an ElemInfo object.
    */
    [[nodiscard]] uint64 GetIndex(ElemInfo elem) const {
        return (elem.row * row_length) + elem.col;
    }

    /**
        * @brief Initialize the matrix with the length it has.
    */
    void InitializeMatrix() {
        if(row_length == -1)
            std::cerr << "ERROR!!! row_length not set";

        for(uint64 m{0}; m < row_length; ++m)
            data[ (m * row_length) + m ] = static_cast<double>(m + 1)/ static_cast<double>(row_length) ;
    }

    /**
        * @brief Initialize the matrix with the given length
        * @param[in] val = the new value of the row_length
    */
    void InitializeMatrix(uint64 val) {
        if(row_length == -1) {
            row_length = val;
            InitializeMatrix();
        }
        else if (val <= 0)
            std::cerr << "ERROR!!! Cannot assign a value <= 0" << std::endl;
        else
            std::cerr << "ERROR!!! Cannot change the row lenght when it "
                      << "has already been initialized" << std::endl;
    }

    /**
        * @brief Return the value at position mtx[row][col]
        * @param[in] row = the row index of the cell.
        * @param[in] col = the column index of the cell.
    */
    [[nodiscard]] double GetValue(uint64 row, uint64 col) const {
        return data[GetIndex(row, col)];
    }

    /**
        * @brief Does mtx[row][col] = val
        * @param[in] row = the row index of the cell.
        * @param[in] col = the column index of the cell.
        * @param[in] val = the value to set in mtx[row][col]
    */
    void SetValue(uint64 row, uint64 col, double val) {
        data[GetIndex(row, col)] = val;
    }

    /**
    * @brief Does mtx[row][col] = val. The main difference is that
    *        here we pass a ElemInfo obj
    * @param[in] elem = reference containing informations
    *                   regarding the element
    * @param[in] val = the value to store
    */
    void SetValue(const ElemInfo& elem, double val) {
        data[GetIndex(elem.row, elem.col)] = val;
    }

    /**
        * @brief Prints the whole matrix
    */
    void PrintMtx() const{
        if(row_length == -1) {
            std::cerr << "ERROR!!! Matrix not yet initialized" << std::endl;
            return;
        }
        if (row_length > 100) {
            std::cerr << "ERROR!!! The matrix is too big to print" << std::endl;
            return;
        }

        std::cout << "\n";
        for(uint64 i = 0; i < row_length; ++i) {
            for(uint64 j = 0; j < row_length; ++j)
                std::cout << data[GetIndex(i, j)] << " ";
            std::cout << "\n";
        }
    }

    /**
        * @brief DEBUG function, fills the matrix with
        *        s.t. ∀ mtx[i][j] = j
    */
    void FillMatrix() {
        for(uint64 i = 0; i < row_length; ++i)
            for (uint64 j = 0; j < row_length; ++j)
                SetValue(i, j, static_cast<double>(j));
    }

    // Parameters
    std::vector<double> data;
    uint64 row_length;
};
#endif //SQUARE_MATRIX_H
