
#include<vector>
#include<iostream>
#include<ff/ff.hpp>
#include<ff/farm.hpp>
#include<thread>
#include "utils/square_matrix.h"

using u64 = std::uint64_t;
using u8 = std::uint8_t;
constexpr u64 default_length = 1 << 14;  // it is read as "2^14"
constexpr u8 default_workers = 4;

/**
 * @brief Represent a task that the Emitter will give to one of
 *        its Workers.
 */

struct Task{
    SquareMtx& mtx;   // Reference to the matrix
    int left_range;           // First element of the diagonal to compute
    int right_range;          // Last element of the diagonal to compute
    int num_diag;             // The upper diagonal where the chunk is
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
    explicit Emitter(SquareMtx& mtx, const int num_workers)
        :mtx(mtx), num_workers(num_workers)
    {PrepareNextDiagonal();}

    /**
        * @brief This method prepare all parameters of the Emitter for computing
        *        the elements of the next matrix.
    */
    void PrepareNextDiagonal() {
        send_tasks = true;
        curr_diag++;
        curr_diag_length = mtx.row_length - curr_diag;

        // Setting Chunk_Size
        chunk_size = (std::ceil(static_cast<float>(curr_diag_length) / num_workers));
        if (chunk_size > 64)    // Like Super Mario hihihi
            chunk_size = 64;
    }

    /**
     * @brief This methods specifies what the Emitter will send to the Workers.
     *        It will be executed at the start of the farm and each time a Worker
     *        returns a value through the Feedback Channel.
     * @param [in] done_wrk = contains the number of elements computed
     *                        by a Worker.
    */
    Task* svc(int* done_wrk) {
        if (done_wrk != nullptr) { // A Worker as returned a message
            curr_diag_length -= *done_wrk;

            if(curr_diag_length <= 0) { // All elements in curr_diag
                                        // have been computed. Starting
                                        // works on the next diagonal.
                PrepareNextDiagonal();
                if(curr_diag >= mtx.row_length) {   // All diagonals have been
                                                    // computed. Closing the farm.
                    eosnotify();
                    return EOS;
                }
            }
        }
        if(send_tasks) {     // Sending Task for next diagonal
            for(int i=0; i < curr_diag_length; i += chunk_size) {   // the first element is 0!!!
                auto* task = new Task{mtx,
                                      i,
                                      i+chunk_size-1,
                                      curr_diag};
                ff_send_out(task);
            }
            send_tasks = false;
            //TODO fai fare un task all'Emitter per non fargli fare attesa attiva
        }
        return GO_ON;
    }

    // Parameters
    SquareMtx& mtx;   // Reference to the matrix to compute.
    int curr_diag{0};           // The current diagonal.
    int curr_diag_length{0};    // Number of elements of the curr. diagonal.
    int chunk_size{0};          // Number of elements per Worker.
    int num_workers{0};         // Number of workers employed by the farm.
    bool send_tasks{false};            // Tells if we have to send tasks to Workers.
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

    int* svc(Task* t) {

        // Checking that the last element isn't out of bounds
        [](int& range, int limit) {
            if(range > limit)
                range = limit;
        }(t->right_range, (t->mtx.row_length - t->num_diag) - 1);

        // Setting parameters
        const int num_elements = t->right_range - t->left_range + 1;
        double temp{0.0};

        // Starting the computation of elements
        for(int k = 0; k < num_elements; ++k) {
            int i = t->left_range + k; int j = t->num_diag + k + t->left_range;

            //Setting parameters for DotProduct
            const int idx_vecr_row = i;  int idx_vecr_col = i;      // Indexes for the first vector
            const int idx_vecc_row = j;  int idx_vecc_col = i + 1;  // Indexes for the second vector
                                                                    // We do not work with the column vector
                                                                    // but with a row in the lower triangular
                                                                    // that contains the same elements
                                                                    // Computing the DotProduct
            for(int elem=0; elem < j-i ; ++elem) {
                temp += t->mtx.GetValue(idx_vecr_row, idx_vecr_col + elem) *
                                t->mtx.GetValue(idx_vecc_row, idx_vecc_col + elem);
            }

            // Storing the result
            t->mtx.SetValue(i, j, std::cbrt(temp));
            t->mtx.SetValue(j, i, std::cbrt(temp));
            temp = 0.0; // Resetting temp
        }
        return new int(num_elements);
    }
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
        mtx_length = std::stoull(argv[1]);

    // Initialize Matrix
    SquareMtx mtx(mtx_length);

    // Setting the num of Workers
    u8 num_workers{default_workers};
    if(std::thread::hardware_concurrency() > 0)
       num_workers = std::thread::hardware_concurrency() -1; // One thread is for the Emitter

    // Creating a Farm with Feedback Channels and no Collector
    Emitter emt{mtx, num_workers};
    ff::ff_Farm<> farm(
            [&]() {
            std::vector<std::unique_ptr<ff::ff_node>> workers;
            for(size_t i=0; i < num_workers; ++i)
                workers.push_back(std::make_unique<Worker>());
            return workers;
        }(),
        emt);
    farm.remove_collector();
    farm.wrap_around();

    // Starting the farm
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
    return 0;
}

