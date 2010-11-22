/*
 * joinFS: jfs_list code, created with sglib macros.
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 */

#if !defined(_REENTRANT)
#define	_REENTRANT
#endif

#include "sglib.h"
#include "jfs_list.h"

#include <stdlib.h>

SGLIB_DEFINE_LIST_FUNCTIONS(jfs_list_t, JFS_LIST_CMP, next)
