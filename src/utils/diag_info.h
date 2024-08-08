//
// Created by Salvatore Salerno on 25/07/24.
//

#ifndef DIAG_INFO_H
#define DIAG_INFO_H
#include<iostream>
// A Struct used to store in a single data object
// all the informations regarding the current diagonal
struct DiagInfo {
    //Constructor
    explicit DiagInfo(int num_of_diagonals)
        :num_of_diagonals(num_of_diagonals)
    {
        current_diag = 0;
        num_elements = num_of_diagonals;
    }

    /**
        * @brief Set internal parameters for handling the next diagonal
    */
    void UpdateDiag() {
        if(!IsLastDiag()) {
            current_diag++;
            num_elements--;
        }
        else
            std::cerr << "Already done all diagonals!" << std::endl;
    }

    /**
        * @brief Returns true if all diagonals have been done
        *        false otherwise
    */
    bool IsLastDiag() {
        return num_of_diagonals - current_diag  == 1;
    }

    // TODO Parameters
    int current_diag{0};        // Is the number of the current diagonal
    // We assume 0 is the major diagonal.

    int num_of_diagonals{0};    // Is the number of diagonals in the
    // matrix

    int length{0};    // Number of elements in current_diag
    int num_elements{0};
};

#endif //DIAG_INFO_H
