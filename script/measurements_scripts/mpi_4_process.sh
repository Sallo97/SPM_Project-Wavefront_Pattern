#!/bin/bash

#SBATCH --job-name=mpi4
#SBATCH --output=../../results/mpi/4_nodes/log/mpi_4_nodes%A_%a.out
#SBATCH --error=../../results/mpi/4_nodes/log/error_mpi_4_nodes%A_%a.err
#SBATCH --time-min=30
#SBATCH --nodes=4

# Number of execution of the program
num_execution=7
start_val=256 #from 256 to 16'384
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "MPI execution 1 task per node, 4 nodes, with argument: $arg"
  mpirun -np 4 --map-by ppr:1:node ../../build/src/parallel_mpi $arg
done