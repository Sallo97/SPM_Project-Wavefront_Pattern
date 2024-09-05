# Compiling sequential code
add_executable(sequential sequential.cpp)

#Compiling FastFlow code (Static Chunk_Size case)
add_executable(parallel_fastflow_static_chunk parallel_fastflow.cpp
        utils/diag_info.h
        utils/compute_range.h)
target_link_libraries(parallel_fastflow_static_chunk PRIVATE Threads::Threads)

#Compiling FastFlow code (Dynamic Chunk_Size case)
add_executable(parallel_fastflow_dynamic_chunk parallel_fastflow.cpp
        utils/diag_info.h
        utils/compute_range.h)
target_link_libraries(parallel_fastflow_dynamic_chunk PRIVATE Threads::Threads)
target_compile_definitions(parallel_fastflow_dynamic_chunk PRIVATE DYNAMIC_CHUNK)

#Compiling MPI Code
add_executable(parallel_mpi parallel_mpi.cpp ${MPI_INCLUDE_PATH}/mpi.h
        utils/diag_info.h
        utils/compute_range.h)
if("${_host_name}" MATCHES "MacBook-Pro-di-Salvatore.local")
    target_link_libraries(parallel_mpi PRIVATE MPI::MPI_CXX ${OpenMP_CXX_LIBRARIES} Threads::Threads)
    target_compile_options(parallel_mpi PUBLIC ${OpenMP_CXX_FLAGS})
else ()
    target_link_libraries(parallel_mpi PRIVATE MPI::MPI_CXX OpenMP::OpenMP_CXX Threads::Threads)
endif ()
