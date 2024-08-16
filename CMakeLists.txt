cmake_minimum_required(VERSION 3.5)
project(SPM_Project)

# Setting constants
set(CMAKE_CXX_STANDARD 20)

include_directories(include)
cmake_host_system_information(RESULT _host_name QUERY HOSTNAME)

if("${_host_name}" MATCHES "MacBook-Pro-di-Salvatore.local")
    MESSAGE("Setting up for my machine")
    set(CMAKE_CXX_COMPILER "/opt/homebrew/Cellar/llvm/18.1.8/bin/clang++")
    set(CMAKE_CXX_FLAGS "-I/opt/homebrew/Cellar/open-mpi/5.0.3_1/include -L/opt/homebrew/Cellar/open-mpi/5.0.3_1/lib")
endif()

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "-Wall -ffast-math -pthread -march=native -finline-functions")
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH} include/ff)

add_subdirectory(src)
