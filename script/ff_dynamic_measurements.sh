#!/bin/bash
#SBATCH --job-name=ff_measurements
#SBATCH --nodes=1
#SBATCH --output=../results/fastflow_dynamic/log/ff_%A_%a.out
#SBATCH --error=../results/fastflow_dynamic/log/error_ff_%A_%a.err
#SBATCH --time=00:50:00 (hrs:min:sec)

# Common params
num_execution=9
start_val=64 #from 64 to 16'384

# Case 4 threads
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "FF Dynamic Chunk execution with argument: $arg"
  ../build/src/parallel_fastflow_dynamic_chunk $arg 4
done

# Case 8 threads
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "FF Dynamic Chunk execution with argument: $arg"
  ../build/src/parallel_fastflow_dynamic_chunk $arg 8
done

# Case 16 threads
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "FF Dynamic Chunk execution with argument: $arg"
  ../build/src/parallel_fastflow_dynamic_chunk $arg 16
done
