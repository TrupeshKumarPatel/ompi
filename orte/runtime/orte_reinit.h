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

// proc list to restart
ORTE_DECLSPEC int orte_reinit_restart_procs( opal_list_t *proc_list );

END_C_DECLS

#endif
