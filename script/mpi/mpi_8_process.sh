#!/bin/bash
#SBATCH --job-name=mpi_8_measurements
#SBATCH --output=../../results/mpi/8_nodes/log/mpi_8_nodes%A_%a.out
#SBATCH --error=../../results/mpi/8_nodes/log/error_mpi_8_nodes%A_%a.err
#SBATCH --time=00:50:00 #50 min (hrs:min:sec)
#SBATCH --nodes=8
#SBATCH --ntasks-per-node=4

# Common params
num_execution=9
start_val=64 #from 64 to 16'384
nodes=8


# 1 Process x Node (8 MPI Process Total)
process_per_node=1
total_processes=$((nodes * process_per_node))
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "MPI execution 1 task per node, 8 nodes, with argument: $arg"
  mpirun -np $total_processes ../../build/src/parallel_mpi $arg
done

# 2 Process x Node (16 MPI Process Total)
process_per_node=2
total_processes=$((nodes * process_per_node))
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "MPI execution 2 task per node, 8 nodes (total 16 processes), with argument: $arg"
  mpirun -np $total_processes ../../build/src/parallel_mpi $arg
done

# 4 Process x Node (32 MPI Process Total)
process_per_node=4
total_processes=$((nodes * process_per_node))
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "MPI execution 4 tasks per node, 8 nodes (total 32 processes), with argument: $arg"
  mpirun -np $total_processes ../../build/src/parallel_mpi $arg
done