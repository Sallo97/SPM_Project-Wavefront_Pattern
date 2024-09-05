#!/bin/bash
#SBATCH --job-name=s_ff_mpi
#SBATCH --time=03:00:00
#SBATCH --nodes=8

# This script executes all the available tests

#Enter directory where scripts are
cd "./measurements_scripts" || { echo "Failed to change directory to ./measurements_scripts"; exit 1; }

# List of measurement script
scripts=(
    "./sequential_measurements.sh"
    "./ff_static_measurements.sh"
    "./ff_dynamic_measurements.sh"
    "./mpi_1_process.sh"
    "./mpi_2_process.sh"
    "./mpi_4_process.sh"
    "./mpi_8_process.sh"
)

# Loop through each script and execute it
for script in "${scripts[@]}"; do
    echo "Running $script..."

    # Make sure the script is executable and execute the script
    chmod +x "$script"
    ./"$script"
done

echo "OK, all tests done successfully!"
