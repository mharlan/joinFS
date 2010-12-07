#ifndef JOINFS_JFS_FILE_CACHE_H
#define JOINFS_JFS_FILE_CACHE_H
/*
 * joinFS - File Module Caching Code
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 *
 * 30 % Demo
 */
#define JFS_FILE_CACHE_SIZE 100

struct jfs_symlink {
  int syminode;
  int datainode;
};

SGLIB_DEFINE_HASHED_CONTAINER_PROTOTYPES(jfs_symlink, JFS_FILE_CACHE_SIZE,
										 jfs_file_cache_function);

#endif
