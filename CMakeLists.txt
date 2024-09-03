cmake_minimum_required(VERSION 3.5)
project(SPM_Project)

# Setting constants
set(CMAKE_CXX_STANDARD 20)

include_directories(include)
cmake_host_system_information(RESULT _host_name QUERY HOSTNAME)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    message(STATUS "Running on macOS")

    if("${_host_name}" MATCHES "MacBook-Pro-di-Salvatore.local")
        # Manually setting compiler and flags
        message(STATUS "Setting up compiler for my machine")
        set(CMAKE_CXX_COMPILER "/opt/homebrew/Cellar/llvm/18.1.8/bin/clang++")
    endif()

    # Manually setting OpenMP
    set(OpenMP_CXX_FLAGS "-fopenmp")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_DEFAULT_MAPPING -DBLOCKING_MODE -DFF_BOUNDED_BUFFER")
    set(OpenMP_CXX_LIB_NAMES "omp")
    set(OpenMP_CXX_LIBRARIES "/opt/homebrew/Cellar/llvm/18.1.8/lib/libomp.dylib")

else()
    MESSAGE("Setting up for SPMCluster")

    #Manually setting MPI
    set(OpenMP_CXX_FLAGS "-pthreads -fopenmp")
    find_package(OpenMP)
endif()


set(CMAKE_BUILD_TYPE Release)
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH} include/ff)
add_subdirectory(src)
