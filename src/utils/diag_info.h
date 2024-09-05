/**
 * @file diag_info.h
 * @brief The DiagInfo class used in the FastFlow and MPI Implementation.
 *        It contains informations regarding the current diagonal of the
 *        matrix being computed.
 * @author Salvatore Salerno
 */


#ifndef DIAG_INFO_H
#define DIAG_INFO_H

#include "constants.h"

/**
 * @brief Contains informations about the current diagonal to compute
 *        when and what diagonal to send to the Workers.
 *        In FastFlow only the Emitter updates it.
 *        In MPI every node has its local copy.
 */
struct DiagInfo {
    /**
     * @brief Constructor of the class
     * @param[in] base_length = the length of the matrix.
     * @param[in] num_workers = the number of Workers in execution.
     */
    explicit DiagInfo(const u64 base_length, const int num_workers) :
        num_actors(num_workers), length(base_length)
    {PrepareNextDiagonal();}

    /**
     * @brief This method prepare all parameters of the Emitter for computing
     *        the elements of the next diagonal of the matrix.
     */
    void PrepareNextDiagonal() {
        num++;
        length--;
    }

    // PARAMETERS
    const int num_actors;
    u64 length{0}; // Number of elements of the curr. diagonal.
    u64 num{0}; // Number of the current diagonal. The major diagonal is counted as 0.
};

#endif // DIAG_INFO_H
