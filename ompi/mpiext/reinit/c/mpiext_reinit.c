/*
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/*
 * This file contains the C implementation of the OMPI_Reinit
 */

#include "ompi_config.h"

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#include "opal/mca/base/base.h"
#include "opal/mca/pmix/pmix.h"
#include "opal/threads/threads.h"

#include "ompi/mca/rte/rte.h"
#include "ompi/runtime/params.h"
#include "ompi/mpi/c/bindings.h"
#include "ompi/mpiext/mpiext.h"
#include "ompi/mpiext/reinit/c/mpiext_reinit_c.h"
#include "ompi/include/ompi/frameworks.h"

#include "ompi/mca/pml/pml.h"
#include "ompi/proc/proc.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/mca/bml/bml.h"
#include "ompi/mca/pml/base/base.h"
#include "ompi/mca/bml/base/base.h"
#include "ompi/communicator/communicator.h"
#include "ompi/mca/coll/base/base.h"
#include "opal/mca/btl/base/base.h"

#define DBG_PRINT(...) fprintf (stderr, "DBG_REINIT: " __VA_ARGS__)

static const char FUNC_NAME[] = "OMPI_Reinit";
static volatile int n_reinit = 0;

static void rollback_on_reinit(int sig)
{
    if( sig == SIGREINIT ) {
        if( opal_reinit_ready) {
            DBG_PRINT("LONGMP!\n");
            opal_reinit_longjmp();
        }
        else {
            DBG_PRINT("Unsupported recovery: fault before Reinit is ready!\n");
            // TODO: Create an MPI_ERR related to Reinit
            ompi_mpi_abort(MPI_COMM_NULL, MPI_ERR_COMM);
        }
   }
}

static void ignore_sigpipe(int sig) {
    DBG_PRINT("Ignoring SIGPIPE!\n");
}

static void fence_release(int status, void *cbdata)
{
    volatile bool *active = (volatile bool*)cbdata;
    OPAL_ACQUIRE_OBJECT(active);
    *active = false;
    OPAL_POST_OBJECT(active);
}

/*************** TAKING OVER ERRORHANDLER *********************/
/* registration callback */
void reinit_proc_err_reg_cb(int status,
        size_t errhandler_ref,
        void *cbdata)
{
    ompi_errhandler_errtrk_t *errtrk = (ompi_errhandler_errtrk_t*)cbdata;

    errtrk->status = status;
    errtrk->active = false;
}

/**
 * Default errhandler callback
 */
void reinit_proc_err_cb(int status,
        const opal_process_name_t *source,
        opal_list_t *info, opal_list_t *results,
        opal_pmix_notification_complete_fn_t cbfunc,
        void *cbdata)
{
    /* tell the event chain engine to go no further - we
     * will handle this */
    if (NULL != cbfunc) {
        cbfunc(OMPI_ERR_HANDLERS_COMPLETE, NULL, NULL, NULL, cbdata);
    }

    DBG_PRINT("%s:%d PROC ERR CB source jobid %u vpid %u status %s\n", __FILE__, __LINE__,
            source->jobid, source->vpid,
            ( OPAL_ERR_PROC_ABORTED == status ? "OPAL_ERR_PROC_ABORTED" :
              ( OPAL_ERR_UNREACH == status ? "OPAL_ERR_UNREACH" : "UNHANDLED ERROR!" ) ) );
    return;
}

void setup_errhandler()
{
    ompi_errhandler_errtrk_t errtrk;
    opal_value_t *kv;
    opal_list_t info, directives;
    errtrk.status = OPAL_ERROR;
    errtrk.active = true;

    /* name it */
    OBJ_CONSTRUCT(&directives, opal_list_t);
    kv = OBJ_NEW(opal_value_t);
    kv->key = strdup(OPAL_PMIX_EVENT_HDLR_NAME);
    kv->type = OPAL_STRING;
    kv->data.string = strdup("REINIT-Proc-Errhandler");
    opal_list_append(&directives, &kv->super);

    OBJ_CONSTRUCT(&info, opal_list_t);
    /* we want to go first */
    /*kv = OBJ_NEW(opal_value_t);
      kv->key = strdup(OPAL_PMIX_EVENT_ORDER_PREPEND);
      opal_list_append(&info, &kv->super);*/
    /* specify the event code */
    kv = OBJ_NEW(opal_value_t);
    kv->key = strdup("status");   // the key here is irrelevant
    kv->type = OPAL_INT;
    kv->data.integer = OPAL_ERR_PROC_ABORTED;
    opal_list_append(&info, &kv->super);

    kv = OBJ_NEW(opal_value_t);
    kv->key = strdup("status2");   // the key here is irrelevant
    kv->type = OPAL_INT;
    kv->data.integer = OPAL_ERR_UNREACH;
    opal_list_append(&info, &kv->super);

    opal_pmix.register_evhandler(&info, &directives, reinit_proc_err_cb,
            reinit_proc_err_reg_cb,
            (void*)&errtrk);

    OMPI_LAZY_WAIT_FOR_COMPLETION(errtrk.active);
}
/************* DONE TAKING OVER ERRORHANDLER *****************/

int OMPI_Reinit(int argc, char **argv, const MPI_Restart_point point)
{
    opal_reinit_disable();
    /*************** SETUP PROC ERROR HANDLER *******************/
    setup_errhandler();
    /************* DONE SETUP PROC ERROR HANDLER *****************/

    // Create signal handler for SIGUSR1
    struct sigaction act;
    act.sa_handler = rollback_on_reinit;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGREINIT, &act, (struct sigaction *)0);
    DBG_PRINT("INSTALLED SIGHANDLER!\n");

    // GG: Ignoring SIGPIPE in case some TCP connection fails
    act.sa_handler = ignore_sigpipe;
    act.sa_flags = 0;
    // XXX: Ignore SIGPIPE, TCP sockets may fail if process fails before restarting
    sigaction(SIGPIPE, &act, (struct sigaction *)0);

    OPAL_SET_REINIT_POINT();

    // XXX: Disable again to avoid races (held locks) after setjmp here
    opal_reinit_disable();
    opal_reinit_ready = true;

    n_reinit++;

    int state;
    OMPI_Reinit_state( &state );

    if( state == OMPI_REINIT_REINITED ) {
        int ret;
        size_t nprocs;
        ompi_proc_t** procs;

        // Replicate ompi_mpi_init barriers to resume
        char *error = NULL;
        volatile bool active;
        bool background_fence = false;

        // modex barrier
        if (NULL != opal_pmix.fence_nb) {
            if (opal_pmix_base_async_modex && opal_pmix_collect_all_data) {
                /* execute the fence_nb in the background to collect
                 * the data */
                background_fence = true;
                active = true;
                OPAL_POST_OBJECT(&active);
                if( OMPI_SUCCESS != (ret = opal_pmix.fence_nb(NULL, true,
                                fence_release,
                                (void*)&active))) {
                    error = "opal_pmix.fence_nb() failed";
                    goto error;
                }

            } else if (!opal_pmix_base_async_modex) {
                /* we want to do the modex */
                active = true;
                OPAL_POST_OBJECT(&active);
                if( OMPI_SUCCESS != (ret = opal_pmix.fence_nb(NULL,
                                opal_pmix_collect_all_data, fence_release, (void*)&active))) {
                    error = "opal_pmix.fence_nb() failed";
                    goto error;
                }
                /* cannot just wait on thread as we need to call opal_progress */
                OMPI_LAZY_WAIT_FOR_COMPLETION(active);
            }
            /* otherwise, we don't want to do the modex, so fall thru */
        } else if (!opal_pmix_base_async_modex || opal_pmix_collect_all_data) {
            if( OMPI_SUCCESS != (ret = opal_pmix.fence(NULL,
                            opal_pmix_collect_all_data))) {
                error = "opal_pmix.fence() failed";
                goto error;
            }
        }

        // GG: disable re-init, it could have been enabled 
        // during opal_progress() within OMPI_LAZY_WAIT_FOR_COMPLETION
        // the following initialization must happen atomically
        opal_reinit_disable();

        // GG: ompi_comm_finalize
        // -> need that to clean expected pml_ob1 header sequences and possibly stale communicators
        if (OMPI_SUCCESS != (ret = ompi_comm_finalize())) {
            assert(0);
        }

        // XXX: Delete all PML procs and re-add them after the barrier
        /* call del_procs on all allocated procs even though some may not be known
         * to the pml layer. the pml layer is expected to be resilient and ignore
         * any unknown procs. */
        // Need to remove procs to update the PML/BML/BTL info
        nprocs = 0;
        procs = ompi_proc_get_allocated (&nprocs);
        MCA_PML_CALL(del_procs(procs, nprocs));

        // GG: comm_init, should it happen before the MPI barrier?
        // -> need that to clean expected pml_ob1 header sequences and possibly stale communicators
        if (OMPI_SUCCESS != (ret = ompi_comm_init())) {
            assert(0);
        }

        /* some btls/mtls require we call add_procs with all procs in the job.
         * since the btls/mtls have no visibility here it is up to the pml to
         * convey this requirement */
        if (mca_pml_base_requires_world ()) {
            if (NULL == (procs = ompi_proc_world (&nprocs))) {
                error = "ompi_proc_get_allocated () failed";
                assert(0);
            }
        } else {
            /* add all allocated ompi_proc_t's to PML (below the add_procs limit this
             * behaves identically to ompi_proc_world ()) */
            if (NULL == (procs = ompi_proc_get_allocated (&nprocs))) {
                error = "ompi_proc_get_allocated () failed";
                assert(0);
            }
        }

        // GG: add procs after comm_init (otherwise vader doesn't work)
        ret = MCA_PML_CALL(add_procs(procs, nprocs));
        free(procs);
        /* If we got "unreachable", then print a specific error message.
           Otherwise, if we got some other failure, fall through to print
           a generic message. */
        if (OMPI_ERR_UNREACH == ret) {
            assert(0);
        } else if (OMPI_SUCCESS != ret) {
            assert(0);
        }

        // GG: add basic communicators
        MCA_PML_CALL(add_comm(&ompi_mpi_comm_world.comm));
        MCA_PML_CALL(add_comm(&ompi_mpi_comm_self.comm));

        // GG: add collective selection
        if (OMPI_SUCCESS !=
                (ret = mca_coll_base_comm_select(MPI_COMM_WORLD))) {
            assert(0);
        }

        if (OMPI_SUCCESS !=
                (ret = mca_coll_base_comm_select(MPI_COMM_SELF))) {
            assert(0);
        }

        // MPI Barrier
        if (background_fence) {
            OMPI_LAZY_WAIT_FOR_COMPLETION(active);
        } else if (!ompi_async_mpi_init) {
            /* wait for everyone to reach this point - this is a hard
             * barrier requirement at this time, though we hope to relax
             * it at a later point */
            if (NULL != opal_pmix.fence_nb) {
                active = true;
                OPAL_POST_OBJECT(&active);
                if (OMPI_SUCCESS != (ret = opal_pmix.fence_nb(NULL, false,
                                fence_release, (void*)&active))) {
                    error = "opal_pmix.fence_nb() failed";
                    goto error;
                }
                OMPI_LAZY_WAIT_FOR_COMPLETION(active);
            } else {
                if (OMPI_SUCCESS != (ret = opal_pmix.fence(NULL, false))) {
                    error = "opal_pmix.fence() failed";
                    goto error;
                }
            }
        }

        DBG_PRINT("STATE: REINITED\n");
error:
        if (ret != OMPI_SUCCESS) {
            /* Only print a message if one was not already printed */
            if (NULL != error && OMPI_ERR_SILENT != ret) {
                const char *err_msg = opal_strerror(ret);
                DBG_PRINT("OMPI_REINIT error: %s\n", err_msg );
            }
            assert(0);
        }
    }
    else if( state == OMPI_REINIT_NEW ) {
        DBG_PRINT("STATE: NEW\n");
    }
    else if( state == OMPI_REINIT_RESTARTED ) {
        DBG_PRINT("STATE: RESTARTED\n");
    }
    else {
        DBG_PRINT("UNKNOWN STATE: %u\n", state );
    }

    opal_reinit_enable();
    point( argc, argv, state );
}

/* ===================================================== */

int OMPI_Reinit_state(int *state)
{
    char *ompi_proc_restart = getenv("OMPI_PROC_RESTART");
    int is_restarted = ( ompi_proc_restart == NULL ? 0 : ( ompi_proc_restart[0] == '1' ? 1 : 0 ) );
    if( n_reinit == 1 ) {
        if( is_restarted) {
            *state = OMPI_REINIT_RESTARTED;
        }
        else {
            *state = OMPI_REINIT_NEW;
        }
    }
    else {
        *state = OMPI_REINIT_REINITED;
    }

    return MPI_SUCCESS;
}

