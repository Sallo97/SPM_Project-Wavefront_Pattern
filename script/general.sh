#!/bin/bash
#SBATCH --job-name=all
#SBATCH --time=03:00:00
#SBATCH --nodes=8

# This script compiles the project, checks for log directories and executes all measurements scripts

# Compile the project
echo "Compiling the project..."
chmod +x "./compile.sh"
./compile.sh
echo "OK, next"

# Create directories where log files will be stored
echo "Checking log directories..."
chmod +rwx "./log_dirs.sh"
./log_dirs.sh
echo "OK, next"

# Execute all steps
echo "Executing all tests..."
chmod +x "./all_measurements.sh"
./all_measurements.sh
echo "OK, All done! BYE BYE!"
