#!/bin/bash
#SBATCH --job-name=all_together
#SBATCH --time=03:00:00 #30 min (hrs:min:sec)
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

# List of measurement script
scripts=(
    "./sequential_measurements.sh"
    "./ff_static_measurements.sh"
    "./ff_dynamic_measurements.sh"
    "./mpi_4_process.sh"
    "./mpi_8_process.sh"
)

# Loop through each script and execute it
cd "./measurements_scripts" || { echo "Failed to change directory to ./measurements_scripts"; exit 1; }
for script in "${scripts[@]}"; do
    echo "Running $script..."

    # Make sure the script is executable and execute the script
    chmod +x "$script"
    ./"$script"
done

echo "OK, all done successfully! BYE BYE!"
