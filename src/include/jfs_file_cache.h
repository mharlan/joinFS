#ifndef JOINFS_JFS_FILE_CACHE_H
#define JOINFS_JFS_FILE_CACHE_H
/*
 * joinFS - File Module Caching Code
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 *
 * 30 % Demo
 * Very simple cache. Not intelligent at all.
 */

/*
 * Initialize the jfs_file_cache.
 *
 * Called when joinFS gets mounted.
 */
void jfs_file_cache_init();

/*
 * Destroy the jfs_file_cache.
 *
 * Called when joinFS gets dismount.
 */
void jfs_file_cache_destroy();

/*
 * Get a datainode from the file cache.
 *
 * Returns 0 if not in the cache.
 */
int jfs_file_cache_get_datainode(int syminode);

/*
 * Get a datainode from the file cache.
 *
 * Returns 0 if not in the cache.
 */
char *jfs_file_cache_get_datapath(int syminode);

/*
 * Add a symlink to the jfs_file_cache.
 */
int jfs_file_cache_add(int syminode, int datainode, const char *datapath);

#endif
