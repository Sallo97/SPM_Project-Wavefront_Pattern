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

#include<stdlib.h>
#include<ff/ff.hpp>
#include<vector>
#include<<cmath>

using namespace ff;

// Represents a task of a Worker in the farm
struct Task {
    //TODO costruttore
 std::vector<double>& mtx; // reference to current matrix
 const int left_range;           // beginning of range chunk
 const int right_rage;           // end of range chunk
 const int row_lenght;           // elements in a row
 const int num_diag;             // diagonal where to compute elements
                                 // (0 is interpreted as the major diagonal)
};


// Send to Workers a chunk of element to compute from
// a diagonal i of the mtx matrix.
// Assuming the total element of the diagonal i is j,
// it waits until it knows through feedback channels
// with the Workers that all elements have been done,
// then it repeat the process for the diagonal i+1
struct Emitter: ff_monode_t<int, Task> {
    // Constructor
    Emitter(std::vector<double>& mtx, int row_length, int num_workers)
        :mtx(mtx), row_length(row_length), num_workers(num_workers){ }

    //TODO Aggiungere commento funzione
    int stc_init() {
        length_diag = row_length;
        chunk_size{(length_diag / num_workers) + 1};
    }

    //TODO Aggiungere commento funzione
    Task* svc(int* done_msg) {
        // First Call
        if(done_msg == nullptr) {
            GiveWork();
            return GO_ON;
        }
        // Subsequent Calls
        else {
            length_diag-chunk_size;  // Wait for all elements to be computed

            if(length_diag == 0) { // All elements of the current diagonal are done.
                                   // Preparing work for the next one.
                curr_diag++;
                length_diag = row_length - curr_diag;
                GiveWork();
                return GO_ON;
            }
        }
    }

    /**
    * @brief It generates a task for each Worker and send it.
     */
    void GiveWork() {
        for(int i = 0; i <= length_diag; i += chunk_size) {
            Task* t = new Task(mtx, i, i + chunk_size - 1, curr_diag);
            ff_send_out (t);
        }
    }

    std::vector<double>& mtx;
    int row_length;
    int curr_diag{0};
    int num_workers{12};
    int chunk_size{1};
    int length_diag{0};
};

// Each Worker receive from the Emitter one or more tasks to compute.
// After it finishes its chunk of tasks it sends to the Emitter
// an integer that is its number of done elements through its
// feedback channel
struct Worker: ff_node_t<Task, int> {

    //TODO aggiungere commento funzione
    int* svc(Task t){
        int i{t.left_range}; int j{t.num_diag+1};

        while (j < t.right_rage-1){
            t.mtx[i*t.row_lenght + j] =
                std::cbrt(
                    [&]() { //TODO considerare caso col ParallelFor
                      auto const elem_vec {j - i};
                      auto sum{0.0};
                      auto const idx_vecr_row{i}; auto idx_vecr_col{i};
                      auto idx_vecc_row{i+1}; auto idx_vecc_col{j};
                      for(auto z{0}; z < elem_vec; ++z) {
                       sum += mtx[idx_vecr_row * n + idx_vecr_col] * mtx[idx_vecc_row * n + idx_vecc_col];
                       idx_vecr_col++;
                       idx_vecc_row++;
                      }
                      return sum;
                   }());
            i++; j++;
        }
    }
};

