#ifndef JOINFS_JFS_FILE_CACHE_H
#define JOINFS_JFS_FILE_CACHE_H
/*
 * joinFS - File Module Caching Code
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 *
 * 30 % Demo
 */

/*
 * Check if the cache has a match.
 */
int jfs_file_cache_has(int key);

/*
 * Return the value associated with the key.
 */
int jfs_file_cache_value(int key);

/*
 * Add a key value pair to the file cache.
 */
int jfs_file_cache_add(int key, int value);

#endif
