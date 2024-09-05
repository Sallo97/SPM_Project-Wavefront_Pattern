#!/bin/bash
#SBATCH --job-name=ff_measurements
#SBATCH --nodes=1
#SBATCH --output=../../results/fastflow_dynamic/log/ff_%A_%a.out
#SBATCH --error=../../results/fastflow_dynamic/log/error_ff_%A_%a.err
#SBATCH --time-min=50

# Common params
num_execution=7
start_val=256 #from 64 to 16'384
bin=$"../../build/src/parallel_fastflow_dynamic_chunk"

# Case 2 threads
for i in $(seq 0 $((6)))
do
  arg=$((start_val * (2 ** i)))
  echo "FF Dynamic Chunk execution with argument: $arg"
  ${bin} $arg 2
done

# Case 4 threads
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "FF Dynamic Chunk execution with argument: $arg"
  ${bin} $arg 4
done

# Case 8 threads
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "FF Dynamic Chunk execution with argument: $arg"
  ${bin} $arg 8
done

# Case 16 threads
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "FF Dynamic Chunk execution with argument: $arg"
  ${bin} $arg 16
done

# Case 32 threads (REMEMBER THAT THE CLUSTER HAS 16 REAL CORE!)
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "FF Dynamic Chunk execution with argument: $arg"
  ${bin} $arg 32
done
