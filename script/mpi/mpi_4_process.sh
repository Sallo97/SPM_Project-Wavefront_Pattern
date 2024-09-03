#!/bin/bash
#SBATCH --job-name=mpi_4_measurements
#SBATCH --output=../../results/mpi/4_nodes/log/mpi_4_nodes%A_%a.out
#SBATCH --error=../../results/mpi/4_nodes/log/error_mpi_4_nodes%A_%a.err
#SBATCH --time=00:50:00 (hrs:min:sec)
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=4

# Common params
num_execution=9
start_val=64 #from 64 to 16'384
nodes=4

# Number of execution of the program
process_per_node=1
total_processes=$((nodes * process_per_node))
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "MPI execution 1 task per node, 4 nodes, with argument: $arg"
  mpirun -np $total_processes ../../build/src/parallel_mpi $arg
done

# Number of execution of the program
process_per_node=2
total_processes=$((nodes * process_per_node))
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "MPI execution 2 tasks per node, 4 nodes (total 8 processes), with argument: $arg"
  mpirun -np $total_processes ../../build/src/parallel_mpi $arg
done