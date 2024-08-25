/**
 * @file parallel_mpi.cpp
 * @brief This code is a parallel implementation of the Wavefront
 *        computation using the MPI library.
 * @details It used a Divide et Impera approach s.t. nodes are divided in Master and Supporter Nodes.
 *          Each node compute its portion of the matrix and returns it to the Master nodes, which merges
 *          the computed matrix. This process continues until the matrix is done.
 * @author Salvatore Salerno
 */

#include <chrono>
#include <iostream>
#include <mpi.h>
#include <vector>
#include "utils/compute_range.h"
#include "utils/constants.h"
#include "utils/diag_info.h"
#include "utils/square_matrix.h"

enum Role { MASTER, SUPPORTER, LAST };

/**
 * @brief Represents a node of the MPI's Wavefront Computations.
 *        Each node can have two roles:
 *          - MASTER = Will merge its matrix with the ones produced by the Supporters
 *          - SUPPORTER = Computed a portion of the matrix and send it to its Master
 */
struct WavefrontNode {
    /**
     * The base constructor of the class.
     * It inizialize a Node and start its WaveFront execution.
     git  @param my_rank = rank of the node
     * @param total_nodes = total number of nodes at beginning of execution
     * @param mtx_length = whole length of the matrix
     */
    WavefrontNode(const int my_rank, const int total_nodes, const u64 mtx_length) :
        my_rank(my_rank), total_nodes(total_nodes), my_mtx(mtx_length)
    {
        active_nodes = total_nodes;
        my_id = my_rank;
        MPIWavefront();
    }

    /**
     * Main Wavefront Computation. It uses a divide et impera approach
     */
    void MPIWavefront() {

        int iteration{0};

        while(true) {
            // [ALL] Update iteration and role
            iteration++;
            SetRole(iteration);
            const u64 sub_mtx_length = static_cast<u64>(std::ceil(my_mtx.length / active_nodes));
            // [ALL] Compute SubMatrix
            ComputeSubMatrix(sub_mtx_length);

            // [SUPPORTER] & [MASTER] associated merge their matrices
            if(my_role == MASTER || my_role == SUPPORTER)
                MergeMatrices(sub_mtx_length, iteration);

            // [LAST] & [SUPPORTER] terminate their execution
            if(my_role == LAST || my_role == SUPPORTER)
                break;

            // [MASTER] UpdateStatus
            UpdateStatus();

            // if(my_rank == 0)
            //     my_mtx.PrintMtx();
        }

    }

    /**
     * @brief Merges the matrices computed by the Master and its adjecent Supporters.
     *        [SUPPORTERS] will send their computation to the Master in a single chunk.
     *        [MASTER] will receive the Supporter's computation and store their results
     *                 in its matrix
     * @return
     */
    void MergeMatrices(const u64 sub_mtx_length, int iteration) {

        // [SUPPORTER] Send to Master the computed matrix
        if(my_role == SUPPORTER) {

            const u64 first_row_col = (my_id * sub_mtx_length);
            const u64 last_row = first_row_col + (sub_mtx_length - 1);
#pragma parallel for
            for(u64 row = first_row_col; row <= last_row; ++row) {
                const u64 offset = my_mtx.GetIndex(row, first_row_col); // the column of the first element is
                                                                       // determined in the same way we compute
                                                                       // the first row, so we reuse the value
                MPI_Send(my_mtx.data.data() + offset,
                        static_cast<int>(sub_mtx_length),
                        MPI_DOUBLE,
                    my_master,
                        0,
                        MPI_COMM_WORLD);
            }
        }

        // [MASTER] Receive from supporter(s) their computed matrix
        else if(my_role == MASTER) {

#pragma parallel for
            for(int i = 0; i < 2; ++i) {
                if(my_supporters[i] != -1) {
                    const u64 first_row = GetId(my_supporters[i], iteration) * sub_mtx_length;
                    const u64 last_row = first_row + (sub_mtx_length-1);
                    auto* send_buff = new MPI_Status{};
#pragma parallel for
                    for(u64 row = first_row; row <= last_row; ++row) {
                        const u64 offset = my_mtx.GetIndex(row, first_row);
                        MPI_Recv(my_mtx.data.data() + offset,
                                static_cast<int>(sub_mtx_length),
                                MPI_DOUBLE,
                                my_supporters[i],
                                0,
                                MPI_COMM_WORLD,
                                send_buff
                                );
                    }
                    // [MASTER] delete allocated buffer
                    delete send_buff;
                }
            }
        }
    }

    /**
     * @brief Compute the submatrix of a Process.
     */
    void ComputeSubMatrix(const u64 sub_mtx_length) {
        // Setting submatrix
        DiagInfo diag{sub_mtx_length, active_nodes};

        // Computing submatrix
        while(diag.num < sub_mtx_length) {
            const u64 start_range = (my_id * sub_mtx_length) + 1;
            const u64 end_range = start_range + (diag.length - 1);
            ComputeRange(start_range, end_range, my_mtx.length, diag.num, my_mtx);
            diag.PrepareNextDiagonal();
        }
    }

    /**
     * @brief Debug function. Prints informations regarding the current status of the process
     */
    void PrintStatus() const {

        std::cout << "Process " << my_rank << " with id " << my_id
                  << " active_nodes " << active_nodes << " and role ";
        if(my_role == MASTER)
            std::cout << "Master with my_supporters = " << my_supporters[0] << " " << my_supporters[1];
        else if(my_role == SUPPORTER)
            std::cout << "Supporter with my_master = " << my_master;
        else
            std::cout << "Last";
        std::cout << std::endl;
    }

    /**
     * @brief determines for the current iteration if the number of active nodes
     *        and the ID of the node
     */
    void UpdateStatus() {
        // [ALL] Update active nodes in current iteration
        active_nodes /= 2;

        // [ALL] Update the id of the node
        my_id /= 2;
    }

    /**
     * Sets the role of the node in the current iteration.
     * It also modifies the arrays my_master and my_supporters accordingly.
     */
    void SetRole(const int iteration) {

        // [LAST] CASE
        if (my_id == 0 && active_nodes == 1) {
            my_role = LAST;
            my_supporters[0] = -1;
            my_supporters[1] = -1;
            my_master = -1;
        }

        // [MASTER] CASE
        else if (my_id % 2 == 0 && (my_id + 1) < active_nodes) {
            my_role = MASTER;
            my_master = -1;
            // Determine supporters rank
            my_supporters[0] = GetRank(my_id + 1, iteration);
            if ((my_id + 2) + 1 == active_nodes)
                my_supporters[1] = GetRank(my_id + 2, iteration);
            else
                my_supporters[1] = -1;
        }

        // [SUPPORTER] CASE
        else {
            my_role = SUPPORTER;
            my_supporters[0] = -1;
            my_supporters[1] = -1;
            // Determining master's rank
            if (my_id % 2 == 0)
                my_master = GetRank(my_id - 2, iteration);
            else
                my_master = GetRank(my_id - 1, iteration);
        }
    }

    /**
     * @brief = Returns the rank of the given id
     * @param id = id to get the rank
     * @param iteration = in which iteration we are
     * @return the rank of the given id
     */
    static int GetRank(const int id, const int iteration) { return static_cast<int>(std::pow(2, iteration - 1) * id); }

    /**
     * @brief = Returns the id of the given rank
     * @param rank = id to get the rank
     * @param iteration = in which iteration we are
     * @return the rank of the given id
     */
    static int GetId(const int rank, const int iteration) { return static_cast<int>(rank / std::pow(2, (iteration - 1))); }

    // Parameters
    int my_id{-1}; // ID in respect to the active nodes. IT IS NOT THE RANK!
    int my_rank{-1}; // the rank of the node in respect to MPI
    int my_role{MASTER}; // Determines if it is either a master of a Worker
    int my_master{-1}; // IF MY ROLE IS SUPPORTED this value is the id of the Master
    std::vector<int> my_supporters{2, -1}; // IF MY ROLE IS MASTER contains the id of the one or two Supporters
    int total_nodes{-1}; // The total number of nodes at beginning of execution
    int active_nodes{-1}; // Active nodes in the current computation
    SquareMtx my_mtx; // Current matrix of the iteration
};

/**
 * @brief A Parallel Wavefront Computation using MPI.
 * @param[in] argv[0] = the length of a row (or column) of the square matrix.
 * @param[in] argc = number of cmd arguments.
 */
int main(int argc, char *argv[]) {
    // [ALL] Setting mtx_length Parameters
    u64 mtx_length{default_length};
    if (argc == 2) // If the user as passed its own lenght
                   // for the matrix use it instead
        mtx_length = std::stoul(argv[1]);

    // [ALL] Initialize MPI
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        std::cerr << "in MPI_Init" << std::endl;
        return EXIT_FAILURE;
    }

    // [ALL] Setting up base params
    int my_rank{0};
    int num_nodes{0};
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);

    if(num_nodes > mtx_length) {
        if(my_rank == 0)
            std::cerr << "ERROR!!! the number of used nodes cannot be greater that the length of the matrix!\n"
                      << "Therefore the program will forcly set the number of nodes equal to the length of the matrix"
                      << std::endl;
        num_nodes = static_cast<int>(mtx_length);
    }

    if (my_rank == PRINCIPAL_RANK)
        std::cout << "Starting MPI_WaveFront Computation with:\n"
                  << "num_nodes = " << num_nodes << "\n"
                  << "mtx.length = " << mtx_length << std::endl;

    // [ALL] Create Node Object
    const auto start = std::chrono::steady_clock::now();
    WavefrontNode my_node{my_rank, num_nodes, mtx_length};

    // [LAST] Print resulting matrix e duration
    if(my_node.my_role == LAST) {
        my_node.my_mtx.PrintMtx();
        const auto end = std::chrono::steady_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Time taken for MPI version: " << duration.count() << " milliseconds" << std::endl;
    }

    // [ALL] Closing MPI Communication and ending the program
    MPI_Finalize();
    return EXIT_SUCCESS;
}