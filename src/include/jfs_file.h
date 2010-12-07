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
 *
 * Returns the inode of the new file or -1;
 */
int jfs_file_create(const char *path, mode_t mode);

/*
 * Create a joinFS static file. The file is added
 * to the Linux VFS and the database.
 *
 * Returns 0 on success, -1 on failure.
 */
int jfs_file_mknod(const char *path, mode_t mode);

/*
 * Delets a joinFS static file.
 */
int jfs_file_unlink(const char *path);

/*
 * Perform a rename on a joinFS file.
 */
int jfs_file_rename(const char *from, const char *to);

/*
 * Truncate a joinFS file.
 */
int jfs_file_truncate(const char *path, off_t size);

/*
 * Open a joinFS file.
 */
int jfs_file_open(const char *path, int flags);

#endif
