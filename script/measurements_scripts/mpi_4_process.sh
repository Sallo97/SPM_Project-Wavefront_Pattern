#!/bin/bash
#SBATCH --job-name=mpi_4_measurements
#SBATCH --output=../../results/mpi/4_nodes/log/mpi_4_nodes%A_%a.out
#SBATCH --error=../../results/mpi/4_nodes/log/error_mpi_4_nodes%A_%a.err
#SBATCH --time=00:50:00
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=4

# Common params
num_execution=7
start_val=256 #from 256 to 16'384
nodes=4
bin=$"../../build/src/parallel_mpi"

# 1 Process x Node (4 MPI Process Total)
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "MPI execution 1 task per node, 4 nodes, with argument: $arg"
  mpirun -np ${nodes} ${bin} $arg
done

# 2 Process x Node (8 MPI Process Total)
process_per_node=2
total_processes=$((nodes * process_per_node))
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "MPI execution 2 tasks per node, 4 nodes (total 8 processes), with argument: $arg"
  mpirun -np ${total_processes} --map-by ppr:${process_per_node}:node ${bin} $arg
done

# 4 Process x Node (16 MPI Process Total)
process_per_node=4
total_processes=$((nodes * process_per_node))
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "MPI execution 4 tasks per node, 8 nodes (total 32 processes), with argument: $arg"
  mpirun -np ${total_processes} --map-by ppr:${process_per_node}:node ${bin} $arg
done