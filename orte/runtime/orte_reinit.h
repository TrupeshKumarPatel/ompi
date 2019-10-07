/**
 * @file
 *
 * Reinit Functionality for the ORTE layer
 */

#ifndef ORTE_REINIT_H
#define ORTE_REINIT_H

#include "orte_config.h"
#include "orte/runtime/orte_globals.h"

BEGIN_C_DECLS

/* define a tracker */
typedef struct {
    opal_object_t super;
    opal_event_t ev;
    orte_proc_t *proc;
} orte_restart_tracker_t;
OBJ_CLASS_DECLARATION(orte_restart_tracker_t);

// proc list to restart
ORTE_DECLSPEC int orte_reinit_send_restart_cmd( opal_list_t *proc_list );
ORTE_DECLSPEC int orte_reinit_restart_proc( orte_proc_t *proc );

END_C_DECLS

#endif
