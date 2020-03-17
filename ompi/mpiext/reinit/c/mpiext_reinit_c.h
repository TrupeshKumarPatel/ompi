/*
 * Copyright (c) 2004-2009 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2011      Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2012 Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

/* This file is included in <mpi-ext.h>.  It is unnecessary to protect
   it from multiple inclusion.  Also, you can assume that <mpi.h> has
   already been included, so all of its types and globals are
   available. */

/*OMPI_DECLSPEC extern int OMPI_Example_global;*/

/* Assumptions 
 * A1: Apps can detect whether to load a CHKPT from mem or disk
 * A2: Apps can detect whether a CHKPT is valid or invalid
 * A3: Apps can determine the latest, consistent CHKPT version
 */

#include <setjmp.h>

typedef enum {
    OMPI_REINIT_NEW = 1,
    OMPI_REINIT_REINITED,
    OMPI_REINIT_RESTARTED
} OMPI_reinit_state_t;

typedef int (*MPI_Restart_point)(int argc, char **argv, OMPI_reinit_state_t state, int num_failed_procs, int *failed_procs);

OMPI_DECLSPEC int OMPI_Reinit_state(int *state);

OMPI_DECLSPEC int OMPI_Reinit(int argc, char **argv, const MPI_Restart_point point);

