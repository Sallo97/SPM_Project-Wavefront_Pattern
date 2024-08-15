cmake_minimum_required(VERSION 3.5)
project(SPM_Project)

# Setting constants
set(CMAKE_CXX_STANDARD 20)

include_directories(include)
cmake_host_system_information(RESULT _host_name QUERY HOSTNAME)

if("${_host_name}" MATCHES "MacBook-Pro-di-Salvatore.local")
    MESSAGE("Setting up for my machine")
    find_package(MPI REQUIRED)
    set(CMAKE_CXX_COMPILER "/opt/homebrew/Cellar/llvm/18.1.8/bin/clang++")
endif()

set(CMAKE_CXX_FLAGS "-Wall -DNDEBUG -ffast-math -pthread -O3 -march=native -finline-functions")
include_directories(${MPI_INCLUDE_PATH} include/ff)
add_subdirectory(src)
