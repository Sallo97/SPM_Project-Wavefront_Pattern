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


// Struct for our Square Matrix
// The matrix is initialized as a matrix of size nxn s.t.
// ∀m in [0, n].mtx[m][m] = (m + 1) / n.
// All other elements in the matrix are set to 0.
struct SquareMtx {
    //Constructor
    explicit SquareMtx(int row_length)
        :data(row_length*row_length, 0), row_length(row_length)
    {
        for(int m{0}; m < row_length; ++m)
            data[ (m * row_length) + m ] = static_cast<double>(m + 1)/ static_cast<double>(row_length) ;
    }

    /**
        * @brief Return the real index where the cell mtx[row][col]
        *        is stored.
        * @param[in] row = the row index of the cell.
        * @param[in] col = the column index of the cell.
    */
    [[nodiscard]] int GetIndex(int row, int col) const {
        return (row * row_length) + col;
    }

    /**
        * @brief Return the value at position mtx[row][col]
        * @param[in] row = the row index of the cell.
        * @param[in] col = the column index of the cell.
    */
    double GetValue(int row, int col) {
        return data[GetIndex(row, col)];
    }

    /**
        * @brief Does mtx[row][col] = val
        * @param[in] row = the row index of the cell.
        * @param[in] col = the column index of the cell.
        * @param[in] val = the value to set in mtx[row][col]
    */
    void SetValue(int row, int col, double val) {
        data[GetIndex(row, col)] = val;
    }
    /**
        * @brief Prints the matrix
    */
    void PrintMtx(){
        std::cout << "\n";
        for(int i = 0; i < row_length; ++i) {
            for(int j = 0; j < row_length; ++j)
                std::cout << data[GetIndex(i, j)] << " ";
            std::cout << "\n";
        }
    }

    // Parameters
    std::vector<double> data;
    int row_length;
};

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
 * @brief Gives to the Worker the elemnts of the upper diagonal
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
                temp += t->mtx.GetValue(idx_vecr_row, idx_vecr_col + elem) * t->mtx.GetValue(idx_vecc_row, idx_vecc_col + elem);
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
 * @brief Determines the number of workers of the farm by determining
 *        the affinity of the hardware
 * @param[in] def = default value
 */
int DetermineWorkers(int def) {
    if(std::thread::hardware_concurrency() > 0)
        return std::thread::hardware_concurrency() -1; // One thread is for the Emitter
    return def;
}

int DetermineLength() {

}

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
        std::cerr << "Invalid lenght! The lenght must be a positive integer != 0"
                  << std::endl;
        return -1;
    }
    const int n{static_cast<int>(std::stoll(argv[1]))};
    // Initialize Matrix
    SquareMtx mtx(n);

    // Setting number of workers
    int num_workers = DetermineWorkers(6);


    // Creating the Farm
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
    if (n > 1) {
        if(farm.run_and_wait_end()<0) {
            std::cerr<<"While running farm";
            return -1;
        }
    }

    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken for parallel version: " << duration.count() << " milliseconds" << std::endl;

    return 0;
}

