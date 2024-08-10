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

/**
 * @brief Represent a task that the Emitter will give to one of
 *        its Workers.
 */

struct Task{
    u16 first_elem;           // First element of the range of elements to compute
    u16 last_elem;            // Last element of the range of elements to compute
    u16 num_diag;             // The upper diagonal where the elements are
                              // We assume that the major diagonal has value 0
};

/**
 * @brief Computes a given chunks of elements of the matrix
 *        following the Wavefront Computation
 * @param[out] mtx = reference of the matrix where compute the elements
 * @param[in] first_elem = reference to the number of the first element in the chunk
 * @param[in] last_elem = reference to the number of the last element in the chunk
 * @param[in] num_diag = reference to the diagonal where the elements are stored
*/
inline void ComputeChunk(SquareMtx& mtx, u16& first_elem,
                         u16& last_elem, u16& num_diag) {
    double temp{0.0};
    for(u16 num_elem = first_elem; num_elem <= last_elem; ++num_elem)
    {
        ElemInfo curr_elem{mtx.length, num_diag, num_elem};
        ComputeElement(mtx, curr_elem, num_diag, temp);

        // Storing the result
        mtx.SetValue(curr_elem, temp);
        mtx.SetValue(curr_elem.col, curr_elem.row, temp);
    }
}

/**
 * @brief Contains informations for the Emitter regarding
 *        when and what diagonal to send to the Workers
 */
struct SendInfo {
    SendInfo(const u16 num_workers, const u16 base_lenght)
    : diag_length(base_lenght), num_workers(num_workers) {PrepareNextDiagonal();}

    /**
    * @brief This method prepare all parameters of the Emitter for computing
    *        the elements of the next diagonal of the matrix.
    */
    void PrepareNextDiagonal() {
        send_tasks = true;
        diag++;
        diag_length--;
        computed_elements = diag_length;

        // Setting Chunk_Size
        chunk_size = static_cast<u16>( (std::ceil(static_cast<float>(diag_length) / static_cast<float>(num_workers) )) );
        // if (chunk_size > default_chunk_size)
        //     chunk_size = default_chunk_size;
    }

    // PARAMETERS
    u16 diag{0};                // The current diagonal.
    u16 diag_length{0};         // Number of elements of the curr. diagonal.
    u16 chunk_size{0};          // Number of elements per Worker.
    u16 num_workers{0};         // Number of elements per Worker.
    u16 computed_elements{0};   // Number of computed elements during a cycle
    bool send_tasks{false};     // Tells if we have to send tasks to Workers.
};

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
struct Emitter: ff::ff_monode_t<int, Task> {
    // Constructor
    explicit Emitter(SquareMtx &mtx, const u16 num_workers)
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
            for(u16 num_elem = 1; num_elem <= send_info.diag_length; num_elem += send_info.chunk_size) {
                auto* task = new Task{num_elem,
                                      num_elem + send_info.chunk_size - 1,
                                      send_info.diag};
                ff_send_out(task);
            }
            send_info.send_tasks = false;
            //TODO fai fare un task all'Emitter per non fargli fare attesa attiva
        }
        return GO_ON;
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
struct Worker: ff::ff_node_t<Task, int> {
    explicit Worker(SquareMtx& mtx) : mtx(mtx) { }

    int *svc(Task *t) override {
        // Checking that the last element isn't out of bounds
        // In case corrects it by setting it to the last
        // element of the diagonal
        [](u16& range, u16 limit) {
            if(range >= limit)
                range = limit;
        }(t->last_elem, (mtx.length - t->num_diag) );

        // Starting computations of elements
        ComputeChunk(mtx, t->first_elem, t->last_elem, t->num_diag);
        return new int (static_cast<int>(t->last_elem - t->first_elem + 1)); // Returing the number
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
    u16 mtx_length{default_length};
    if (argc >= 2) // If the user as passed its own lenght
                   // for the matrix use it instead
        mtx_length = std::stoull(argv[1]);

    // Setting the num of Workers
    u8 num_workers{default_workers};
    if(std::thread::hardware_concurrency() > 0)
       num_workers = std::thread::hardware_concurrency() -1; // One thread is for the Emitter
    if (argc >=3)   // If the user passed its own num of
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