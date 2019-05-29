# -*- shell-script -*-
#
# Copyright (c) 2004-2009 The Trustees of Indiana University.
#                         All rights reserved.
# Copyright (c) 2012-2015 Cisco Systems, Inc.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

# OMPI_MPIEXT_reinit_CONFIG([action-if-found], [action-if-not-found])
# -----------------------------------------------------------
AC_DEFUN([OMPI_MPIEXT_reinit_CONFIG],[
    AC_CONFIG_FILES([ompi/mpiext/reinit/Makefile])

    AC_CONFIG_FILES([ompi/mpiext/reinit/c/Makefile])
    AC_CONFIG_FILES([ompi/mpiext/reinit/mpif-h/Makefile])
    AC_CONFIG_FILES([ompi/mpiext/reinit/use-mpi/Makefile])
    AC_CONFIG_FILES([ompi/mpiext/reinit/use-mpi-f08/Makefile])

    # If your extension can build, run $1.  Otherwise, run $2.  For
    # the purposes of this reinit, we don't want it to build in most
    # cases.  So only build if someone specifies an --enable-mpi-ext
    # value that contains the token "reinit".
    AS_IF([test "$ENABLE_reinit" = "1" || \
           test "$ENABLE_EXT_ALL" = "1"],
          [$1],
          [$2])
])

# only need to set this if the component needs init/finalize hooks
AC_DEFUN([OMPI_MPIEXT_reinit_NEED_INIT], [1])
