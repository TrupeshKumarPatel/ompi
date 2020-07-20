#!/bin/bash

if [ -z $1 ]; then
    echo "Usage: ./run.sh <executable>"
    exit 0
fi

rm -f chkp-*.txt
salloc -N 2 --ntasks-per-node=1 -ppdebug \
mpirun -np 2 --mca orte_enable_recovery 1 --mca pml ^cm --mca plm_slurm_args "--wait 0" \
    ./mpi_allreduce \
    |& tee log-$1.txt

