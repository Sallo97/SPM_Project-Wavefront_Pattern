#!/bin/bash
#SBATCH --job-name=mpi_measurements
#SBATCH --output=../results/mpi/mpi_%A_%a.out
#SBATCH --error=../results/mpi/error_mpi_%A_%a.err
#SBATCH --time=02:00:00 #2hr (hrs:min:sec)

# Number of execution of the program
num_execution=10
start_val=64 #from 64 to 32'768
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "MPI execution with argument: $arg"
  mpirun ../build/src/parallel_mpi $arg
done
