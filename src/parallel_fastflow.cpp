/**
 * @file parallel_fastflow.cpp
 * @brief This code is a parallel implementation of the Wavefront
 *        computation using the FastFlow library.
 * @details It initialize a matrix of dimension NxN (parameter given as input).
 *          At the beginning the values are all set to 0 except
 *          the ones in the major diagonal s.t.:
 *          ∀i in [0, N].mtx[i][i] = (i + 1) / n.
 *          Then all elements in the upper part of the major diagonal are computed s.t.:
 *          mtx[i][j] = square_cube( dot_product(v_m, v_m+k) ).
 *          To parallelize it uses a Farm with Feedback Channels
 * @author Salvatore Salerno
 */

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include "../include/ff/ff.hpp"
#include "../include/ff/farm.hpp"
#include "./utils/compute_range.h"
#include "./utils/constants.h"
#include "./utils/diag_info.h"
#include "./utils/square_matrix.h"

/**
 * @brief Computes a given chunks of elements of the matrix
 *        following the Wavefront Computation
 * @param id_chunk = recognizes which range of the elems to compute, start at position 1
 * @param diag = obj containing information regarding the current diagonal
 * @param mtx = the matrix where to gather and store result
 * @param chunk_sizes = contains the chunk_size to use.
 */
inline void ComputeChunk(const int id_chunk, const DiagInfo &diag, SquareMtx &mtx, std::vector<u64> &chunk_sizes) {
    // Getting chunk_sizes:
    // Dynamic case : its dynamic and depends on the associated id_chunk (note that id_chunk stats at 1 while chunks start at 0
    // Static case : chunk_sizes are all equal thus just get the one at position 0 in the array
#ifdef DYNAMIC_CHUNK
    u64 chunk_size = chunk_sizes[id_chunk - 1];
#else
    u64 chunk_size = chunk_sizes[0];
#endif // DYNAMIC_CHUNK

    // Determining the range
    // Be aware that elements start at position 1
    u64 const start_range = ((id_chunk - 1) * chunk_size) + 1;
    u64 end_range = start_range + (chunk_size - 1);
    if (end_range > diag.length || id_chunk == diag.num_actors) // Hotfix Out of Bounds
        end_range = diag.length;

    ComputeRange(start_range, end_range, diag.length, diag.num, mtx);
}


/**
 * @brief Gives to the Worker the elements of the upper diagonal
 *        to compute following the Wavefront Pattern.
 *        Assuming there are z elements in diagonal i, each
 *        Worker will return a value i that specify the number
 *        of elements it computed. When all elements have been
 *        computed the Emitter sends Tasks relative to the i+1
 *        diagonal.
 */
struct Emitter final : ff::ff_monode_t<u8, u8> {
    // Constructor
    explicit Emitter(SquareMtx &mtx, DiagInfo &diag, const u8 num_workers, std::vector<u64> &chunk_sizes) :
        num_workers(num_workers), mtx(mtx), diag(diag), chunk_sizes(chunk_sizes) {
        send_tasks = true;
    }

    /**
     * @brief It will be executed at the start of the farm and each time a Worker
     *        returns a value through the Feedback Channel.
     * @param [in] task_done = the task object returned by the Worker
     */
    u8 *svc(u8 *task_done) override {

        // A Worker has returned a message
        if (task_done != nullptr) {
            delete task_done;
            active_workers--;

            // Check if all elements of the current diagonal
            // have been computed. If so start work on the next one.
            if (active_workers == 0) {
                diag.PrepareNextDiagonal();
                send_tasks = true;

                // If all diagonals have been computed we can terminate the farm
                if (diag.num >= mtx.length) {
                    eosnotify();
                    return EOS;
                }
            }
        }

        // Computing a new diagonal by sending Tasks to Workers
        if (send_tasks) {
            SendTasks();

            // Compute its part (if it has any)
            if (id_emitter != -1)
                ComputeChunk(id_emitter, diag, mtx, chunk_sizes);
        }

        return GO_ON;
    }

    /**
     * @brief Sends tasks to Workers that tells them to start work on the next diagonal.
     */
    void SendTasks() {
        // Setting base params
        u64 elems_to_send = diag.length;
        u64 chunk_size = std::ceil((static_cast<double>(diag.length) / static_cast<double>(num_workers + 1)));
        if (chunk_size == 0)
            chunk_size = 1;
        chunk_sizes[0] = chunk_size;

        // Sending tasks until all elements of the matrix have been distributed
        while (elems_to_send >= chunk_size && active_workers < num_workers) {
            // Sending tasks
            auto *id_chunk = new u8{static_cast<u8>(active_workers + 1)};
            ff_send_out(id_chunk);

            // Updating params
#ifdef DYNAMIC_CHUNK
            if(active_workers != 0) // The first case is already stored
                chunk_sizes[active_workers] = chunk_size;
#endif // DYNAMIC_CHUNK

            if (elems_to_send <= chunk_size) // Avoid out of bounds
                elems_to_send = 0;
            else
                elems_to_send -= chunk_size;
            active_workers++;

#ifdef DYNAMIC_CHUNK
            // Recomputes dynamically the chunk_size
            chunk_size = static_cast<u64>(std::ceil(elems_to_send / ((num_workers - active_workers) + 1)));
            if (chunk_size == 0) // Out of Bounds fix
                chunk_size = 1;
#endif // DYNAMIC_CHUNK

        }

        // Setting id_chunk for the Emitter
        if (elems_to_send == 0)
            id_emitter = -1;
        else
            id_emitter = active_workers + 1;

        // Update flag to be aware that for the current diagonal all tasks have been sent
        send_tasks = false;
    }

    // Parameters
    bool send_tasks{false}; // Tells if we have to send tasks to Workers.
    int id_emitter{0}; // Indicates which part of the diagonal the Emitter needs to compute
    u8 active_workers{0}; // Active workers doing a computation
    u8 num_workers{0}; // Total workers in the farm
    SquareMtx &mtx; // Reference to the matrix to compute.
    DiagInfo &diag; // Contains informations for what and when
                    // to send tasks to Workers
    std::vector<u64> &chunk_sizes;  // Reference to the chunk_size vector
};

/**
 * @brief Receive from the Emitter the task to compute.
 *        A task gives information to the elements of the
 *        matrix the Worker needs to compute. After it computes
 *        them, it send through a Feedback Channel the number of
 *        elements it computed.
 */
struct Worker final : ff::ff_node_t<u8> {
    explicit Worker(SquareMtx &mtx, DiagInfo &diag, std::vector<u64> &chunk_sizes) : mtx(mtx), diag(diag),
        chunk_sizes(chunk_sizes){ id_chunk = 0; }

    u8 *svc(u8 *task) override {
        id_chunk = *task;

        // Computing elemes
        ComputeChunk(id_chunk, diag, mtx, chunk_sizes);

        // Discarding arrived token and sending a new one
        return task;
    }

    // PARAMS
    SquareMtx &mtx; // Reference of the matrix to compute
    DiagInfo &diag; // Contains informations updated by the Emitter regarding the diagonal to compute
    u8 id_chunk{0}; // Identifies which chunk of elems the Worker will compute, id_chunk in [1, num_workers]
    u64 start_range{0}; // Value used during a diag computation for telling the first element of the range
    u64 end_range{0}; // Value used during a diag computation for telling the last element of the range
    double temp{0.0}; // Value where to store temporary computations for an element
    std::vector<u64> &chunk_sizes; // Reference of the array containg the chunk size
};

/**
 * @brief Returns the num of workers to be used in the program.
 * @param[in] argv[1] is the length a row (or a column) of the matrix
 * @param[in] argc = the number of cmd arguments.
 * @note If no argument is passed, then we assume the matrix has default length
 */
inline u8 ReturnNumThreads(const int argc, char *argv[]) {
    // Checking if the value has been passed in the CMD
    if (argc >= 3)
        return std::stoul(argv[2]);

    // If not try to use the maximum capacity of the Hardware
    if (std::thread::hardware_concurrency() > 0)
        return std::thread::hardware_concurrency();

    // If not use the default value
    return default_workers;
}

/**
 * @brief A Parallel Wavefront Computation using the library FastFlow.
 *        It uses a Farm with Feedback Channels.
 * @param[in] argv[1] is the length a row (or a column) of the matrix (OPTIONAL)
 * @param[in] argv[2] is the number of workers (OPTIONAL)
 * @param[in] argc = the number of cmd arguments.
 * @note If no argument is passed, then we assume the matrix has default length
 */
int main(const int argc, char *argv[]) {
    // Setting matrix length
    u64 mtx_length{default_length};
    if (argc >= 2) // If the user as passed its own lenght
                   // for the matrix use it instead
        mtx_length = std::stoul(argv[1]);

    const u8 num_threads = ReturnNumThreads(argc, argv);

    std::cout << "mtx_lenght = " << mtx_length << "\n"
              << "hardware_concurrency = " << std::thread::hardware_concurrency() << "\n"
              << "num_threads = " << static_cast<int>(num_threads) << std::endl;

    // Initialize common params
    SquareMtx mtx{mtx_length};
    DiagInfo diag{mtx.length, num_threads};
    std::vector<u64>chunk_sizes(num_threads, 0); // Contains the chunk size for each chunk_id

    // Creating a Farm with Feedback Channels and no Collector
    Emitter emt{mtx, diag, static_cast<u8>(num_threads - 1), chunk_sizes};
    ff::ff_Farm<> farm(
            [&](const u8 num_workers) {
                std::vector<std::unique_ptr<ff::ff_node>> workers;
                for (u8 i = 0; i < num_workers; ++i)
                    workers.push_back(std::make_unique<Worker>(mtx, diag, chunk_sizes));
                return workers;
            }(num_threads - 1),
            emt);
    farm.remove_collector();
    farm.wrap_around();

    // Starting the WaveFront Pattern
    const auto start = std::chrono::steady_clock::now();

    // If we have a 1x1 matrix, then we do not need
    // to apply the computation
    if (mtx_length > 1) {
        if (farm.run_and_wait_end() < 0) {
            std::cerr << "ERROR!!! An error occurred while running the farm";
            return EXIT_FAILURE;
        }
    }

    const auto end = std::chrono::steady_clock::now();

    // Printing duration and closing program
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken for FastFlown version: " << duration.count() << " milliseconds" << std::endl;
    return EXIT_SUCCESS;
}