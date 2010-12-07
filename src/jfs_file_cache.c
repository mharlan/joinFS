/*
 * joinFS - File Module Caching Code
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 *
 * 30 % Demo
 */

#include "sglib.h"
#include "jfs_file_cache.h"

SGLIB_DEFINE_HASHED_CONTAINER_FUNCTIONS(jfs_symlink, JFS_FILE_CACHE_SIZE,
										jfs_file_cache_function);
