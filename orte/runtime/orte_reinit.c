/** @file
 *
 * ORTE Layer Reinit Runtime functions
 *
 */

#include "orte/runtime/orte_reinit.h"
#include "orte/mca/odls/odls_types.h"
#include "orte/mca/grpcomm/grpcomm.h"
#include "orte/mca/errmgr/errmgr.h"

static int _send_proc_restart_bcast( opal_list_t *proc_list )
{
    int rc;
    opal_buffer_t *cmd;
    orte_daemon_cmd_flag_t command = ORTE_DAEMON_RESTART_PROCS;
    orte_grpcomm_signature_t *sig;

    /*OPAL_OUTPUT_VERBOSE((5, orte_plm_base_framework.framework_output,
                         "%s plm:base:orted_cmd sending restart_proc cmd",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));*/

    cmd = OBJ_NEW(opal_buffer_t);
    /* pack the command */
    if (ORTE_SUCCESS != (rc = opal_dss.pack(cmd, &command, 1, ORTE_DAEMON_CMD))) {
        ORTE_ERROR_LOG(rc);
        OBJ_RELEASE(cmd);
        return rc;
    }

    uint32_t num_procs = opal_list_get_size( proc_list );
    printf("SENDING num_procs %u processes to restart\n", num_procs );

    /* pack the number of process */
    if (ORTE_SUCCESS != (rc = opal_dss.pack(cmd, &num_procs, 1, OPAL_UINT32))) {
        ORTE_ERROR_LOG(rc);
        OBJ_RELEASE(cmd);
        return rc;
    }

    orte_proc_t *proc;
    OPAL_LIST_FOREACH( proc, proc_list, orte_proc_t ) {
        printf("SENDING proc %s to restart on daemon %u\n", ORTE_NAME_PRINT(&proc->name), proc->parent);
        /* pack the parent daemon vpid */
        if (ORTE_SUCCESS != (rc = opal_dss.pack(cmd, &proc->parent, 1, ORTE_VPID))) {
            ORTE_ERROR_LOG(rc);
            OBJ_RELEASE(cmd);
            return rc;
        }

        /* pack the proc name */
        if (ORTE_SUCCESS != (rc = opal_dss.pack(cmd, &proc->name, 1, ORTE_NAME))) {
            ORTE_ERROR_LOG(rc);
            OBJ_RELEASE(cmd);
            return rc;
        }
    }

    /* goes to all daemons */
    sig = OBJ_NEW(orte_grpcomm_signature_t);
    sig->signature = (orte_process_name_t*)malloc(sizeof(orte_process_name_t));
    sig->signature[0].jobid = ORTE_PROC_MY_NAME->jobid;
    sig->signature[0].vpid = ORTE_VPID_WILDCARD;
    if (ORTE_SUCCESS != (rc = orte_grpcomm.xcast(sig, ORTE_RML_TAG_DAEMON, cmd))) {
        ORTE_ERROR_LOG(rc);
    }
    OBJ_RELEASE(cmd);
    OBJ_RELEASE(sig);

    /* we're done! */
    return rc;
}

int orte_reinit_restart_procs( opal_list_t *proc_list )
{
    _send_proc_restart_bcast( proc_list );
}

