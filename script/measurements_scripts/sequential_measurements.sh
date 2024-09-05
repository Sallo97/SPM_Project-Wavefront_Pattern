#!/bin/bash
#SBATCH --job-name=seq
#SBATCH --nodes=1
#SBATCH --output=../../results/sequential/log/seq_%A_%a.out
#SBATCH --error=../../results/sequential/log/error_seq_%A_%a.err
#SBATCH --time-min=50

# Number of execution of the program
num_execution=7
start_val=256 #from 256 to 16'384
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "Sequential execution with argument: $arg"
  ../../build/src/sequential $arg
done
