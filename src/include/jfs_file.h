#ifndef JOINFS_JFS_FILE_H
#define JOINFS_JFS_FILE_H
/*
 * joinFS - File Module
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 *
 * 30 % Demo
 */

#include <fuse.h>

int jfs_s_file_open(const char *path, struct fuse_file_info *fi);
int jfs_s_file_read(const char *path, char *buf, size_t size, off_t offset,
					struct fuse_file_info *fi);
int jfs_s_file_write(const char *path, const char *buf, size_t size,
					 off_t offset, struct fuse_file_info *fi);
int jfs_s_file_truncate(const char *path, off_t size);
int jfs_s_file_access(const char *path, int mask);

#endif
