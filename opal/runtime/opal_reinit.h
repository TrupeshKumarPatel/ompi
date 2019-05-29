/**
 * @file
 *
 * Reinit functionality for Open MPI
 */

#include "opal_config.h"

#ifndef OPAL_REINIT_H
#define OPAL_REINIT_H

#include <signal.h>
#include <setjmp.h>

BEGIN_C_DECLS

#define SIGREINIT SIGUSR1
OPAL_DECLSPEC extern bool opal_reinit_ready;
OPAL_DECLSPEC void opal_reinit_enable();
OPAL_DECLSPEC void opal_reinit_disable();
OPAL_DECLSPEC void opal_reinit_longjmp();

extern sigjmp_buf env;

#define OPAL_SET_REINIT_POINT() \
{                               \
    sigsetjmp( env, 1 );        \
}

#define OPAL_REINIT_ENTER_LIBRARY() { opal_reinit_disable(); }
#define OPAL_REINIT_EXIT_LIBRARY()  { opal_reinit_enable(); }
END_C_DECLS

#endif /* OPAL_REINIT_H */

