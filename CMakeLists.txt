cmake_minimum_required(VERSION 3.5)
project(SPM_Project)

# Setting constants
set(CMAKE_CXX_STANDARD 20)

include_directories(include)
cmake_host_system_information(RESULT _host_name QUERY HOSTNAME)

MESSAGE("Machine = " "${_host_name}")

if("${_host_name}" MATCHES "MacBook-Pro-di-Salvatore.local")
    MESSAGE("Setting up for my machine")

    # Manually setting compiler and flags
    set(CMAKE_CXX_COMPILER "/opt/homebrew/Cellar/llvm/18.1.8/bin/clang++")
    set(CMAKE_CXX_FLAGS "-I/opt/homebrew/Cellar/open-mpi/5.0.3_1/include -L/opt/homebrew/Cellar/open-mpi/5.0.3_1/lib")

    # Manually setting OpenMP
    set(OpenMP_CXX_FLAGS "-fopenmp")
    set(OpenMP_CXX_LIB_NAMES "omp")
    set(OpenMP_CXX_LIBRARIES "/opt/homebrew/Cellar/llvm/18.1.8/lib/libomp.dylib")

elseif("${_host_name}" MATCHES "spmln|node0[0-9]")
    MESSAGE("Setting up for SPMCluster")

    #Manually setting MPI
    set(CMAKE_CXX_FLAGS "-I/opt/ohpc/pub/mpi/openmpi5-gnu12/5.0.3/include -L/opt/ohpc/pub/mpi/openmpi5-gnu12/5.0.3/lib -Wl,-rpath -Wl,/opt/ohpc/pub/mpi/openmpi5-gnu12/5.0.3/lib -Wl, -lmpi")
    set(OpenMP_CXX_FLAGS "-fopenmp")
    find_package(OpenMP)
endif()

set(CMAKE_BUILD_TYPE Release)
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH} include/ff)
add_subdirectory(src)
