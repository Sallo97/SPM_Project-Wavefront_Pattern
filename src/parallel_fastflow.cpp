
#include<vector>
#include<iostream>
#include<ff/ff.hpp>
#include<ff/farm.hpp>
#include<thread>
#include "utils/square_matrix.h"
#include "utils/constants.h"

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
        curr_diag_length = mtx.length - curr_diag;

        // Setting Chunk_Size
        chunk_size = static_cast<u16>( (std::ceill(static_cast<float>(curr_diag_length) / num_workers)) );
        if (chunk_size > default_chunk_size)
            chunk_size = default_chunk_size;
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
                if(curr_diag >= mtx.length) {   // All diagonals have been
                                                    // computed. Closing the farm.
                    eosnotify();
                    return EOS;
                }
            }
        }
        if(send_tasks) {     // Sending Task for next diagonal
                             // Elements start at 1
            for(u16 i=1; i <= curr_diag_length; i += chunk_size) {   // the first element is 0!!!
                auto* task = new Task{i,
                                      i+chunk_size - 1,
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
    u16 curr_diag{0};           // The current diagonal.
    u16 curr_diag_length{0};    // Number of elements of the curr. diagonal.
    u16 chunk_size{0};          // Number of elements per Worker.
    u8 num_workers{0};         // Number of workers employed by the farm.
    bool send_tasks{false};            // Tells if we have to send tasks to Workers.
};

/**
 * @brief Compute for an element mtx[i][j] the DotProduct of the vectors
 *        mtx[i][j] = cube_root( DotProd(row[i],col[j]) ).
 *        The dot product is done on elements already computed
 * @param[in] mtx = the reference to the matrix.
 * @param[in] elem = contains information regarding the element
 *                   that we need to compute.
 * @param[in] vec_length = the length of the vectors of the DotProduct
 *                         (usually is equal to the diagonal where the elem is from).
 * @param[out] res = where to store the result
*/
inline void ComputeElement(SquareMtx& mtx, ElemInfo& elem,
                       u16& vec_length, double& res) {
    res = 0.0; // Reset the result value
    const ElemInfo fst_elem_vec_row{elem.GetVecRowElem()}; // Indexes for the first vector

    const ElemInfo fst_elem_vec_col{elem.GetVecColElem()}; // Indexes for the second vector
    // In reality We do not work with the column vector
    // but with a row in the lower triangular
    // that contains the same elements

    // Starting the DotProduct Computation
    for(u16 i = 0; i < vec_length; ++i)
        res += mtx.GetValue(fst_elem_vec_row.row, fst_elem_vec_row.col + i)
                * mtx.GetValue(fst_elem_vec_col.row, fst_elem_vec_col.col + i);

    // Storing the cuberoot of the final result
    res = std::cbrt(res);
}

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

    int* svc(Task* t) {

        // Checking that the last element isn't out of bounds
        // In case correct it
        [](u16& range, u16 limit) {
            if(range >= limit)
                range = limit;
        }(t->last_elem, (mtx.length - t->num_diag) );


        // Setting parameters
        const int diag_length = t->last_elem - t->first_elem + 1;
        temp = 0.0; // Resetting temp

        std::cout   << "num_diag = " << t->num_diag
                    << " first_elem = " << t->first_elem
                    << " last_elem = " << t->last_elem << std::endl;


        // // Starting computations of elements
        for(u16 num_elem = t->first_elem; num_elem <= t->last_elem; ++num_elem)    //TODO POSSIBILE ERRORE QUI SUL <=
        {
            ElemInfo curr_elem{mtx.length, t->num_diag, num_elem};
            ComputeElement(mtx, curr_elem, t->num_diag, temp);

            // Storing the result
            mtx.SetValue(curr_elem, temp);
            mtx.SetValue(curr_elem.col, curr_elem.row, temp);
        }

        // [OLD] Starting the computation of elements
        // for(int k = 0; k < diag_length; ++k) {
        //     int i = t->first_elem + k; int j = t->num_diag + k + t->first_elem;
        //
        //     //Setting parameters for DotProduct
        //     const int idx_vecr_row = i;  int idx_vecr_col = i;      // Indexes for the first vector
        //     const int idx_vecc_row = j;  int idx_vecc_col = i + 1;  // Indexes for the second vector
        //                                                             // We do not work with the column vector
        //                                                             // but with a row in the lower triangular
        //                                                             // that contains the same elements
        //                                                             // Computing the DotProduct
        //     for(int elem=0; elem < j-i ; ++elem) {
        //         temp += mtx.GetValue(idx_vecr_row, idx_vecr_col + elem) *
        //                         mtx.GetValue(idx_vecc_row, idx_vecc_col + elem);
        //     }
        //
        //     // Storing the result
        //     mtx.SetValue(i, j, std::cbrt(temp));
        //     mtx.SetValue(j, i, std::cbrt(temp));
        //     temp = 0.0; // Resetting temp
        // }
        return new int(diag_length);
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
    std::cout << "num_workers = " << static_cast<int>(num_workers) << "\n"
              << "mtx_length = " << mtx_length << std::endl;


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
    mtx.PrintMtx();
    return 0;
}

