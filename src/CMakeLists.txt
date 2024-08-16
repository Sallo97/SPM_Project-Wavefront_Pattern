# Compiling sequential code
add_executable(sequential sequential.cpp)

#Compiling FastFlow code
add_executable(parallel_fastflow parallel_fastflow.cpp)

#Compiling MPI Code
add_executable(parallel_mpi parallel_mpi.cpp ${MPI_INCLUDE_PATH}/mpi.h)
target_link_libraries(parallel_mpi MPI::MPI_CXX)