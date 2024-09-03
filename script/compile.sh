#!/bin/bash

# This script compiles the project. The binaries are inside the build/src directory

# Storing the dir path where the script are stored
script_dir=$(pwd)

# Configuring FastFlow according to the architecture of the machine
echo "Executing FastFlow Mapping String"
fastflow_dir="../include/ff"
cd "$fastflow_dir" || { echo "Failed to change directory to $fastflow_dir"; exit 1; }
echo "Y" | ./mapping_string.sh

# Compiling the project
cd "../../." || { echo "Failed to change directory to ../../."; exit 1; }
cmake -B build

cd "./build" || { echo "Failed to change directory to ./build"; exit 1; }
make

# Return to the original directory
cd "$script_dir" || { echo "Failed to return to the original directory"; exit 1; }

