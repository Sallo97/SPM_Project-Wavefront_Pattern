/**
* @file parallel_fastflow.cpp
 * @brief This code is a parallel implementation of the Wavefront computation using the FastFlow library.
 * @details It initialize a matrix of dimension NxN (parameter given as input).
 *          All values are set to 0 except the one in the major diagonal s.t.:
 *          ∀i in [0, N].mtx[i][i] = (i + 1) / n.
 *          Then all elements in the upper part of the major diagonal are computed s.t.:
 *          mtx[i][j] = square_cube( dot_product(v_m, v_m+k) ).
 *          This is parallelized by using a farm with feedback channels.
 *          The Emitter send to the worker a chunck of elements of the current diagonal to compute.
 *          The Workers compute them and send a "done" message to the Emitter.
 *          When all elements of the diagonal have been computer, the Emitter repeat
 *          the process for the next diagonal.
 * @author Salvatore Salerno
 */

#include<vector>
#include<iostream>
#include<ff/ff.hpp>
#include<ff/farm.hpp>
#include<thread>

using u64 = uint64_t;

void print_matrix(std::vector<double>& mtx, u64 row_length) {
    std::cout<<std::endl<<std::endl;
    for(u64 i=0; i < row_length; i++) {
        for(u64 j=0; j < row_length; j++) {
            std::cout<<mtx[i*row_length + j]<<" ";
        }
        std::cout<<std::endl;
    }
}

/**
 * @brief Represent a task that the Emitter will give to one of
 *        its Workers.
 *
 * This struct contains:
 * - mtx = a reference to the matrix.
 * - left_range = the first element of the chunk.
 * - right_range = the last element of the chunk.
 * - num_diag = the upper diagonal where the chunk is.
 *              Assume that -1 is the num_diag for the
 *              major diagonal.
 */

struct Task{
    std::vector<double>& mtx;   // Reference to the matrix
    u64 left_range;           // First element of the diagonal to compute
    u64 right_range;          // Last element of the diagonal to compute
    u64 num_diag;             // The upper diagonal where the chunk is
    u64 row_length;           // The length of a row (or column) of mtx
};

/**
 * @brief Gives to the Worker the elemnts of the upper diagonal
 *        to compute following the Wavefront Pattern.
 *        Assuming there are z elements in diagonal i, each
 *        Worker will return a value i that specify the number
 *        of elements it computed. When all elements have been
 *        computed the Emitter sends Tasks relative to the i+1
 *        diagonal.
 * Return Type: Task, it gives one to a Worker
 * Receiver Type: u64, it gets one from a Worker.
 *                It represents the number of computed elements.
 */
struct Emitter: ff::ff_monode_t<u64, Task> {
    // Constructor
    explicit Emitter(std::vector<double>& mtx, u64 row_length,
                     u64 num_workers)
        :mtx(mtx), row_length(row_length),num_workers(num_workers)
        {
        curr_diag = 0;  // We assume the major diagonal is 0
        chunk_size = 0;
        send_tasks = true;
        curr_diag_length = 0;
    }

    /**
     * @brief This methods specifies what the Emitter will send to the Workers.
     *        It will be executed at the start of the farm and each time a Worker
     *        returns a value through the Feedback Channel.
     * @param [in] done_wrk = contains the number of elements computed
     *                        by a Worker.
    */
    Task* svc(u64* done_wrk) {
        if (done_wrk != nullptr) { // A Worker as returned a message
            curr_diag_length -= *done_wrk;
            delete(done_wrk);

            if(curr_diag_length <= 0) { // All elements in curr_diag
                                        // have been computed. Starting
                                        // works on the next diagonal.
                if(curr_diag >= row_length - 1) {   // All diagonals have been
                                                // computed. Closing the farm.
                    eosnotify();
                    return EOS;
                }
                send_tasks = true;
            }
        }

        if(send_tasks) {     // Sending Task for next diagonal
            // Setting Parameters
            send_tasks = false;
            curr_diag++;
            curr_diag_length = row_length - curr_diag;

            // Setting Chunk_Size
            chunk_size = (std::ceil(static_cast<float>(curr_diag_length) / num_workers));
            if (chunk_size > 64)    // Like Super Mario hihihi
                chunk_size = 64;

            // Sending Tasks
            for(u64 i=0; i < curr_diag_length; i += chunk_size) {   // the first element is 0!!!
                auto* task = new Task{mtx,
                                      i,
                                      i+chunk_size-1,
                                      curr_diag,
                                      row_length};
                ff_send_out(task);
            }
            //TODO fai fare un task all'Emitter per non fargli fare attesa attiva
        }
        return GO_ON;
    }

    // Parameters
    std::vector<double>& mtx;   // Reference to the matrix to compute.
    u64 row_length;          // The Lenght of a row (or column) of mtx.
    u64 curr_diag;           // The current diagonal.
    u64 curr_diag_length;    // Number of elements of the curr. diagonal.
    u64 chunk_size;          // Number of elements per Worker.
    u64 num_workers;         // Number of workers employed by the farm.
    bool send_tasks;            // Tells if we have to send tasks to Workers.
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
struct Worker: ff::ff_node_t<Task, u64> {
    u64* svc(Task* t) {

        // Checking that right_range isn't out of bounds
        if(t->right_range > (t->row_length - t->num_diag) - 1)
            t->right_range = (t->row_length - t->num_diag) - 1;

        // Setting parameters
        u64 num_elements = t->right_range - t->left_range + 1;
        double temp{1.0};

        // Starting the computation of elements
        for(u64 k = 0; k < num_elements; ++k) {
            u64 i = t->left_range + k; u64 j = t->num_diag + k + t->left_range;

            //Setting parameters for DotProduct
            const u64 idx_vecr_row = i;  u64 idx_vecr_col = i;      // Indexes for the first vector
            const u64 idx_vecc_row = j;  u64 idx_vecc_col = i + 1;  // Indexes for the second vector
                                                                    // We do not work with the column vector
                                                                    // but with a row in the lower triangular
                                                                    // that contains the same elements
                                                                    // Computing the DotProduct
            for(u64 elem_vec=0; elem_vec < j-i ; ++elem_vec) {
                temp += t->mtx[idx_vecr_row * t->row_length + idx_vecr_col] *
                                  t->mtx[idx_vecc_row * t->row_length + idx_vecc_col];
                idx_vecr_col++;
                idx_vecc_col++;
            }

            // Storing the result

            t->mtx[i * t->row_length + j] = t->num_diag;//std::cbrt(temp);
            t->mtx[j * t->row_length + i] = t->num_diag;//std::cbrt(temp);
        }
        delete(t);
        return new u64(num_elements);
    }
};

/**
 * @brief A Parallel Wavefront Computation using the library FastFlow.
 *        It uses a Farm with Feedback Channels.
 * @param[in] argv[1] is the length a row (or a column) of the matrix
 * @exception argc < 3
 */
int main(int argc, char* argv[]) {
    // Setting length row/column matrix
    if(argc!=2) {
        std::cerr << "You must pass as argument the lenght of the square matrix to generate"
                  << std::endl;
        return -1;
    }
    if (std::stoll(argv[1]) <= 0) {
        std::cerr << "Invalid lenght! The lenght must be a positive u64eger != 0"
                  << std::endl;
        return -1;
    }
    const u64 n{static_cast<u64>(std::stoll(argv[1]))};

    // Setting number of workers
    u64 num_workers{6};
    if(std::thread::hardware_concurrency() > 0)
        num_workers = std::thread::hardware_concurrency() -1; // One thread is for the Emitter

    // Initialize mtx as a matrix of size nxn s.t.
    // ∀m in [0, n].mtx[m][m] = (m + 1) / n.
    // All other elements in the matrix are set to 0.
    std::vector<double> mtx{static_cast<double>(n * n), 0};
    for(u64 m{0}; m < n; ++m)
        mtx[ (m * n) + m ] = static_cast<double>(m + 1)/ static_cast<double>(n) ;
    //print_matrix(mtx,n);   //TODO REMOVE CALL

    // Creating the Farm
    ff::ff_Farm<> farm(
        [&]() {
            std::vector<std::unique_ptr<ff::ff_node>> workers;
            for(size_t i=0; i < num_workers; ++i)
                workers.push_back(std::make_unique<Worker>());
            return workers;
        }()
    );
    Emitter emt{mtx, n, num_workers};
    farm.add_emitter(emt);
    farm.remove_collector();
    farm.wrap_around();

    // Starting the farm
    ff::ffTime(ff::START_TIME);
    if(farm.run_and_wait_end()<0) {
        std::cerr<<"While running farm";
        return -1;
    }
    ff::ffTime(ff::STOP_TIME);
    std::cout << "Time taken for parallel version: " << ff::ffTime(ff::GET_TIME) << " milliseconds" << std::endl;
    return 0;
}

