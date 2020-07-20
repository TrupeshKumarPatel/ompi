#!/bin/bash

rm -f chkp-*.txt
salloc -N 6 --ntasks-per-node=1 -ppdebug \
mpirun -np 4 --mca orte_enable_recovery 1 --mca pml ^cm --mca plm_slurm_args "--wait 0" \
    ./mpi_cascade \
    |& tee log-cascade.txt
    #--mca btl_openib_allow_ib 1 \
    #--mca orte_debug_daemons 1 \
    #--mca btl_base_verbose 9 \

