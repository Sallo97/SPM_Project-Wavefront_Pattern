cmake_minimum_required(VERSION 3.28)
project(Project)

set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR}/include)

# Set the optimization flag
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

add_executable(Project #src/main.cpp
        #src/sequential.cpp
        src/parallel_fastflow.cpp
)

# Find the Threads package
find_package(Threads REQUIRED)

# Link the pthread library
target_link_libraries(Project PRIVATE Threads::Threads)
