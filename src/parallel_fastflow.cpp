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
#include "utils/elem_info.h"
#include "utils/ff_send_info.h"
#include "utils/ff_task.h"


/**
 * @brief Represent a task that the Emitter will give to one of
 *        its Workers.
 */

/**
 * @brief Computes a given chunks of elements of the matrix
 *        following the Wavefront Computation
 * @param[out] mtx = reference of the matrix where compute the elements
 * @param[in] start_range = reference to the number of the first element in the chunk
 * @param[in] end_range = reference to the number of the last element in the chunk
 * @param[in] num_diag = reference to the diagonal where the elements are stored
*/
inline void ComputeChunk(SquareMtx& mtx, const u64& start_range, const u64& end_range, const u64& num_diag) {
    double temp{0.0};
    for(u64 num_elem = start_range; num_elem <= end_range; ++num_elem)
    {
        ElemInfo curr_elem{mtx.length, num_diag, num_elem};
        ComputeElement(mtx, curr_elem, num_diag, temp);

        // Storing the result
        mtx.SetValue(curr_elem, temp);
        mtx.SetValue(curr_elem.col, curr_elem.row, temp);
    }
}

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
struct Emitter final: ff::ff_monode_t<int, Task> {
    // Constructor
    explicit Emitter(SquareMtx &mtx, const u64 num_workers)
        : mtx(mtx), send_info(num_workers, mtx.length){ }

    /**
     * @brief This methods specifies what the Emitter will send to the Workers.
     *        It will be executed at the start of the farm and each time a Worker
     *        returns a value through the Feedback Channel.
     * @param [in] done_wrk = contains the number of elements computed
     *                        by a Worker.
     */
    Task *svc(int *done_wrk) override {

        // A Worker has returned a message
        if (done_wrk != nullptr) {

            send_info.computed_elements -= *done_wrk;
            delete done_wrk;

            // Check if all elements of the current diagonal
            // have been computed. If so start work on the next one.
            if(send_info.computed_elements <= 0) {
                send_info.PrepareNextDiagonal();

                // If all diagonals have been computed we can terminate the farm
                if(send_info.diag >= mtx.length) {
                    eosnotify();
                    return EOS;
                }
            }
        }

        // Computing a new diagonal by sending Tasks to Workers
        // Be aware that elements starts from position 1
        if(send_info.send_tasks) {
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
        u64 elems_to_send = send_info.diag_length;
        u64 remaining_workers = send_info.num_workers;
        u64 start_range{1};

        // Sending tasks until all elements of the matrix have been distributed
        while(elems_to_send > 0 && remaining_workers > 0) {

            // Determining chunk_size
            const u64 chunk_size = std::ceil(elems_to_send / remaining_workers);
            // if(chunk_size < default_chunk_size)
            //     chunk_size = default_chunk_size;

            // Determining range of elems for task
            u64 end_range = start_range + (chunk_size - 1);
            if(end_range >= send_info.diag_length)
                end_range = send_info.diag_length;

            // Sending task
            auto* t = new Task{start_range, end_range, send_info.diag};
            ff_send_out(t);

            // Updating params
            if(elems_to_send <= chunk_size)
                elems_to_send = 0;
            else
                elems_to_send -= chunk_size;

            // Updating base params
            remaining_workers--;
            start_range = end_range + 1;
        }

        // Update flag to be aware that for the current diagonal all tasks have been sent
        send_info.send_tasks = false;
    }

    // Parameters
    SquareMtx& mtx;            // Reference to the matrix to compute.
    SendInfo send_info;        // Contains informations for what and when
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
struct Worker final: ff::ff_node_t<Task, int> {
    explicit Worker(SquareMtx& mtx) : mtx(mtx) { }

    int *svc(Task *t) override {
        // Starting computations of elements
        ComputeChunk(mtx, t->start_range, t->end_range, t->diag);

        // Dealloc task pointer
        const int computed_elems = static_cast<int>(t->end_range - t->start_range) + 1;
        delete t;

        // Send num of computed elements to Emitter
        return new int (computed_elems); // Returning the number
                                         // of computed elements
    }

    // PARAMETERS
    SquareMtx& mtx; // Reference to the matrix
    double temp{0.0}; // Stores the results of
                      // the computaton of an Element
};

/**
 * @brief A Parallel Wavefront Computation using the library FastFlow.
 *        It uses a Farm with Feedback Channels.
 * @param[in] argv[1] is the length a row (or a column) of the matrix
 * @param[in] argc = the number of cmd arguments.
 * @note If no argument is passed, then we assume the matrix has default length
 */
int main(int argc, char* argv[]) {
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

    // Initialize Matrix
    SquareMtx mtx(mtx_length);

    // Creating a Farm with Feedback Channels and no Collector
    Emitter emt{mtx, num_workers};
    ff::ff_Farm<> farm(
            [&]() {
            std::vector<std::unique_ptr<ff::ff_node>> workers;
            for(size_t i=0; i < num_workers; ++i)
                workers.push_back(std::make_unique<Worker>(mtx));
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
        if(farm.run_and_wait_end()<0) {
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