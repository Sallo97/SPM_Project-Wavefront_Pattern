cmake_minimum_required(VERSION 3.28)
project(Project)

set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR}/include)

# Find MPI package
find_package(MPI REQUIRED)

# Include directories
include_directories(${MPI_INCLUDE_PATH})

# Set the optimization flag
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

add_executable(Project
        src/utils/square_matrix.h
        src/utils/elem_info.h
        src/sequential.cpp
)

# Link the MPI library
target_link_libraries(Project MPI::MPI_C)

