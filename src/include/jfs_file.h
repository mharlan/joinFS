#ifndef JOINFS_JFS_FILE_H
#define JOINFS_JFS_FILE_H
/*
 * joinFS - File Module
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 *
 * 30 % Demo
 */
#include <fuse.h>

/*
 * Create a joinFS static file. The file is added
 * to the Linux VFS and the database.
 */
int jfs_s_file_create(const char *path, int q_inode, mode_t mode);

/*
 * Gets the datainode associated with a joinFS static file symlink.
 *
 * Returns the datainode or -1.
 */
int jfs_s_file_get_datainode(const char *path);


#endif
