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

#include<vector>
#include<iostream>
#include<chrono>
#include<ff/ff.hpp>
#include<ff/farm.hpp>
#include<thread>
#include "utils/square_matrix.h"
#include "utils/constants.h"
#include "utils/compute_elem.h"
#include "utils/ff_diag_info.h"

//TODO ENUM FOR TYPE OF TASK



/**
 * @brief Gives to the Worker the elements of the upper diagonal
 *        to compute following the Wavefront Pattern.
 *        Assuming there are z elements in diagonal i, each
 *        Worker will return a value i that specify the number
 *        of elements it computed. When all elements have been
 *        computed the Emitter sends Tasks relative to the i+1
 *        diagonal.
 * Return Type: Task, it gives one to a Worker
 * Receiver Type: int, it gets one from a Worker.
 *                It represents the number of computed elements.
 */
struct Emitter final: ff::ff_monode_t<u8, u8> {
    // Constructor
    explicit Emitter(SquareMtx &mtx, DiagInfo& diag)
        : mtx(mtx), diag(diag){send_tasks = true;}

    /**
     * @brief This methods specifies what the Emitter will send to the Workers.
     *        It will be executed at the start of the farm and each time a Worker
     *        returns a value through the Feedback Channel.
     * @param [in] done_wrk = contains the number of elements computed
     *                        by a Worker.
     */
    u8 *svc(u8 *done_wrk) override {

        // A Worker has returned a message
        if (done_wrk != nullptr) {
            delete done_wrk;
            active_workers--;

            // Check if all elements of the current diagonal
            // have been computed. If so start work on the next one.
            if(active_workers == 0) {
                diag.PrepareNextDiagonal();
                send_tasks = true;

                // If all diagonals have been computed we can terminate the farm
                if(diag.num >= mtx.length) {
                    eosnotify();
                    return EOS;
                }
            }
        }

        // Computing a new diagonal by sending Tasks to Workers
        if(send_tasks) {
            SendTasks();
            //TODO fai fare un task all'Emitter per non fargli fare attesa attiva
        }
        return GO_ON;
    }

    /**
     * @brief Sends tasks (i.e. range of elems of mtx) to Workers to compute the
     *        current diagonal. The elems are equally distributed among the threads,
     *        determining it for every task dynamically each time a new Task is been computed

     */
    void SendTasks() {
        // Setting base params
        u64 elems_to_send = diag.length;

        // Sending tasks until all elements of the matrix have been distributed
        while(elems_to_send > 0 && active_workers < diag.num_workers) {
            // Sending tasks
            auto* id_chunk = new u8{static_cast<u8>(active_workers + 1)};
            ff_send_out(id_chunk);


            // Updating params
            if(elems_to_send <= diag.chunk_size) // Hotfix to avoid out of bound
                elems_to_send = 0;
            else
                elems_to_send -= diag.chunk_size;
            active_workers ++;
        }

        // Update flag to be aware that for the current diagonal all tasks have been sent
        send_tasks = false;
    }

    // Parameters
    bool send_tasks{false};      // Tells if we have to send tasks to Workers.
    u8 active_workers{0};        // Workers doing a computation
    SquareMtx& mtx;              // Reference to the matrix to compute.
    DiagInfo& diag;              // Contains informations for what and when
                                 // to send tasks to Workers
};

/**
 * @brief Receive from the Emitter the task to compute.
 *        A task gives information to the elements of the
 *        matrix the Worker needs to compute. After it computes
 *        them, it send through a Feedback Channel the number of
 *        elements it computed.
 * Receiver Type: Task*, it gets one from the Emitter.
 *                It represents the number of computed elements.
 */
struct Worker final: ff::ff_node_t<u8> {
    explicit Worker(SquareMtx& mtx, DiagInfo& diag)
        : mtx(mtx), diag(diag){ id_chunk = 0; }

    u8* svc(u8* task) override {
        id_chunk = *task;

        // Computing elemes
        ComputeChunk();

        // Discarding arrived token and sending a new one
        delete task;
        return new u8;
    }

    /**
     * @brief Computes a given chunks of elements of the matrix
     *        following the Wavefront Computation
     */
    void ComputeChunk() {
        // Determining the range
        // Be aware that elements start at position 1
        start_range = ((id_chunk - 1) * diag.chunk_size) + 1;
        end_range = start_range + (diag.chunk_size - 1);
        if (end_range > diag.length || id_chunk == diag.num_workers) // Hotfix Out of Bounds
            end_range = diag.length;

        // If start_range is out of bounds skip computation
        if (start_range > diag.length)
            return;

        // Starting Computation
        temp = 0.0;
        while(start_range <= end_range){

            // Determining element
            // ElemInfo curr_elem{mtx.length, num_diag, start_range};
            u64 row = start_range - 1;
            u64 col = row + diag.num;

            // Computing element
            ComputeElement(mtx, row, col, diag.num, temp);

            // Storing the result
            mtx.SetValue(row, col, temp);
            mtx.SetValue(col, row, temp);

            // Setting next element to compute
            ++start_range;
        }
    }

    // PARAMS
    SquareMtx& mtx;     // Reference of the matrix to compute
    DiagInfo& diag;     // Contains informations updated by the Emitter regarding
                        // the diagonal to compute
    u8 id_chunk{0};     // Identifies which chunk of elems the Worker will compute, id_chunk in [1, num_workers]
    u64 start_range{0}; // Value used during a diag computation for telling the first element of the range
    u64 end_range{0};   // Value used during a diag computation for telling the last element of the range
    double temp{0.0};   // Value where to store temporary computations for an element

};

/**
 * @brief A Parallel Wavefront Computation using the library FastFlow.
 *        It uses a Farm with Feedback Channels.
 * @param[in] argv[1] is the length a row (or a column) of the matrix
 * @param[in] argc = the number of cmd arguments.
 * @note If no argument is passed, then we assume the matrix has default length
 */
int main(const int argc, char* argv[]) {
    // Setting matrix length
    u64 mtx_length{default_length};
    if (argc >= 2) // If the user as passed its own lenght
        // for the matrix use it instead
            mtx_length = std::stoul(argv[1]);

    // Setting the num of Workers
    u8 num_workers{default_workers};
    if(std::thread::hardware_concurrency() > 0)
        num_workers = std::thread::hardware_concurrency() -1; // One thread is for the Emitter
    if (argc >=3)                                            // If the user passed its own num of
        // Workers use it instead
            num_workers = std::stoul(argv[2]);

    std::cout << "mtx_lenght = " << mtx_length << "\n"
              << "hardware_concurrency = " << std::thread::hardware_concurrency() << "\n"
              << "num_workers = " << static_cast<int>(num_workers) << std::endl;

    // Initialize Matrix and Diag
    SquareMtx mtx{mtx_length};
    DiagInfo diag{mtx.length, num_workers};

    // Creating a Farm with Feedback Channels and no Collector
    Emitter emt{mtx, diag};
    ff::ff_Farm<> farm(
            [&]() {
            std::vector<std::unique_ptr<ff::ff_node>> workers;
            for(size_t i=0; i < num_workers; ++i)
                workers.push_back(std::make_unique<Worker>(mtx, diag));
            return workers;
        }(),
        emt);
    farm.remove_collector();
    farm.wrap_around();

    // Starting the WaveFront Pattern
    const auto start = std::chrono::steady_clock::now();

    // If we have a 1x1 matrix, then we do not need
    // to apply the computation
    if (mtx_length > 1) {
        if(farm.run_and_wait_end() < 0) {
            std::cerr<<"ERROR!!! An error occurred while running the farm";
            return -1;
        }
    }

    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken for FastFlown version: " << duration.count() << " milliseconds" << std::endl;
    mtx.PrintMtx();
    return 0;
}