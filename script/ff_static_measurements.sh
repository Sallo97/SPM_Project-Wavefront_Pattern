#!/bin/bash
#SBATCH --job-name = ff_measurements
#SBATCH --output = ../results/fastflow_static/ff_%A_%a.out
#SBATCH --error = ../results/fastflow_static/error_ff_%A_%a.err
#SBATCH --time = 02:00:00 #2hr (hrs:min:sec)

# Case 4 threads
num_execution=10
start_val=64 #from 64 to 32'768
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "FF Static Chunk execution with argument: $arg"
  ../build/src/parallel_fastflow_static_chunk $arg 4
done

# Case 8 threads
num_execution=10
start_val=64 #from 64 to 32'768
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "FF Static Chunk execution with argument: $arg"
  ../build/src/parallel_fastflow_static_chunk $arg 8
done

# Case 16 threads
num_execution=10
start_val=64 #from 64 to 32'768
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "FF Static Chunk execution with argument: $arg"
  ../build/src/parallel_fastflow_static_chunk $arg 16
done
