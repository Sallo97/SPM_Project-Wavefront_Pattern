#!/bin/bash
#SBATCH --job-name = sequential_measurements
#SBATCH --output = ../results/sequential/seq_%A_%a.out
#SBATCH --error = ../results/sequential/error_seq_%A_%a.err
#SBATCH --time = 03:00:00 #2hr (hrs:min:sec)

# Number of execution of the program
num_execution=10
start_val=64 #from 64 to 32'768
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "Sequential execution with argument: $arg"
  ../build/src/sequential $arg
done
