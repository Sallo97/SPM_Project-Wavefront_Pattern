# Distributed Wavefront Computation [SPM 23/24]
This repository contains my project for the Parallel Distributed Computing course (SPM) at the University of Pisa.
The project focuses on implementing a Wavefront Pattern using two different parallel programming paradigms: 
- A shared memory solution using the FastFlow library
- A distributed memory solution using MPI and OpenMP.
## Brief Description
In this project, the algorithm computes a square matrix by following a Wavefront pattern across its upper diagonal.

The process is as follows: given an upper-triangular square matrix mtx of size n x n, only the elements on the
main diagonal are known initially:
```angular2html
∀ i ∈ [0, n - 1]: mtx[i][i] = (i + 1) / n
```
For the each of the upper diagonals, the elements are computed as:
```angular2html
∀ i ∈ [0, n - 1], ∀ j ∈ [i + 1, n - 1]:
mtx[i][j] = ³√( Σ (from k = 0 to j - 1) ( mtx[i][k] * mtx[j][k + 1] ) )
```

## Compilation
Start by cloning the following repository:
```bash
$ git clone https://github.com/Sallo97/SPM_Project-Wavefront_Pattern
$ cd SPM_Project-Wavefront_Pattern-main
```
For compiling the project it is necessary the FastFlow library, MPI Library and OpenMP. 
The FastFlow library is already installed inside the 'include' directory.
If it is the first time that the FastFlow library is used on the machine, it is necessary to configure it calling the 'mapping_string' script:
```bash
$ cd ./include/ff
$ ./mapping_string.sh
```
Inside the root directory, with CMake create the `build` folder, and inside it generates the necessary build files based on your system configuration. To do so, run the following command:
```bash
$ cmake -S . -B build
```

Inside the generated  build folder, the project can be compiler by both ninja and make:
```bash
$ cd build
$ make
```
Finished compilation the binaries are inside the subfolder of build called 'src'. The programs are and are:
- **sequential [lenght of a row/column of the matrix]** = a sequential execution of the pattern
- **parallel_fastflow_static_chunk [length of a row/column of the matrix] [num of theads to spawn]** = a parallel execution using FastFlow 
which uses static chunk sizes.
- **parallel_fastflow_dynamic_chunk [length of a row/column of the matrix] [num of theads to spawn]** = a parallel execution using FastFlow
  which uses dynamic chunk sizes.
- **parallel_mpi [lenght of a row/column of the matrix]** = parallel execution utilizing MPI and OMP.
### Scripts
In the root directory inside the 'script' folder are present some script that tests the various binary. 

### More Information
More information regarding the implementation and performance measurements of the project are present in the 
report stored in the 'report' directory.