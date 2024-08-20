# Compiling sequential code
add_executable(sequential sequential.cpp)

#Compiling FastFlow code
add_executable(parallel_fastflow parallel_fastflow.cpp
        utils/mpi_myinfo.h
        utils/diag_info.h
        utils/compute_range.h)

#Compiling MPI Code
add_executable(parallel_mpi parallel_mpi.cpp ${MPI_INCLUDE_PATH}/mpi.h
        utils/mpi_myinfo.h
        utils/diag_info.h
        utils/compute_range.h)
target_link_libraries(parallel_mpi MPI::MPI_CXX ${OpenMP_CXX_FLAGS})
target_compile_options(parallel_mpi PUBLIC ${OpenMP_CXX_FLAGS})