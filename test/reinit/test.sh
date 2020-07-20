#!/bin/bash
#MSUB -l walltime=00:15:00
#MSUB -q pbatch
#MSUB -e reinit_bcast.err
#MSUB -o reinit_bcast.out
#MSUB -l nodes=1
mpirun -np 8 --mca orte_enable_recovery 1 ./mpi_allreduce
