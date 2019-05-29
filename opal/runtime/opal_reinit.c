/** @file
 *
 * OPAL Layer Reinit Runtime functions
 *
 */

#include <signal.h>

#include "opal/runtime/opal_reinit.h"

// Global var for opal_reinit
bool opal_reinit_ready = false;
sigjmp_buf env;

void opal_reinit_longjmp()
{
    siglongjmp( env, 1 );
}

void opal_reinit_disable(void)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGREINIT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
}

void opal_reinit_enable(void)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGREINIT);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}
