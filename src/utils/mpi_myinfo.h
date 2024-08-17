/**
* @file mpi_myinfo.h
 * @brief The MyInfo class used in the MPI Implementation.
 *        It contains parameters local to each process.
 * @author Salvatore Salerno
 */

#ifndef MPI_MYINFO_H
#define MPI_MYINFO_H

#include "constants.h"

/** @brief Contains all necessary parameters for a MPI process
 */
struct MyInfo {

    /** @brief Base constructor
     * @param[in] my_rank = rank of the process
     * @param[in] num_processes = number of processes in the application
     * @param[in] max_length = max length of local_row and local_col
     */
    MyInfo(const int my_rank, int num_processes, const u64 max_length) :
        my_rank(my_rank), num_processes(num_processes),
        row_displs(num_processes, 0), col_displs(num_processes, 0), counts(num_processes, 0),
        local_row(max_length, 0.0), local_col(max_length, 0.0) { }

    /** @brief Sets ScatterV arrays to 0 for all processes out of bounds so that they do not do any computation
     * @param[in] first_process = rank of the first process out of bounds.
     */
    void SetOutOfBounds(const int first_process) {
        for (int i = first_process; i < num_processes; ++i) {
            row_displs[i] = 0;
            col_displs[i] = 0;
            counts[i] = 0;
        }
    }

    /** @brief Returns true if the associated process is the Master,
     *         false instead.
    */
    [[nodiscard]] bool AmIMaster() const {
        return (my_rank == MASTER_RANK);
    }

    /** @brief Debug function that prints all various stored in the arrays of the obj
     */
    void DebugPrint() const {

        std::cout << "printing counts of Process " << my_rank << "\n";
        for (auto x: counts)
            std::cout << x << " ";

        std::cout << "\nprinting row_displs of Process " << my_rank << "\n";
        for (auto x: row_displs)
            std::cout << x << " ";

        std::cout << "\nprinting col_displs of Process " << my_rank << "\n";
        for (auto x: col_displs)
            std::cout << x << " ";

        std::cout << "\nprinting local_row of Process " << my_rank << "\n";
        for (u64 i = 0; i < MyCount(); ++i)
            std::cout << local_row[i] << " ";

        std::cout << "\nprinting local_col of Process " << my_rank << "\n";
        for (u64 i = 0; i < MyCount(); ++i)
            std::cout << local_col[i] << " ";
        std::cout << std::endl;
    }

    /**
     * @brief Returns the number of elements of the LocalDotProduct Computation of a Process.
     *        It is the value stored in the associated entry in counts
     *
     */
    [[nodiscard]] u64 MyCount() const {
        return counts[my_rank];
    }

    // PARAMETERS
    int my_rank;
    int num_processes;
    std::vector<int> row_displs;
    std::vector<int> col_displs;
    std::vector<int> counts;
    std::vector<double> local_row;
    std::vector<double> local_col;
};

#endif // MPI_MYINFO_H
