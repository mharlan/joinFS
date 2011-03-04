/********************************************************************
 * Copyright 2010, 2011 Matthew Harlan <mharlan@gwmail.gwu.edu>
 *
 * This file is part of joinFS.
 *	 
 * JoinFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * JoinFS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with joinFS.  If not, see <http://www.gnu.org/licenses/>.
 ********************************************************************/

#ifndef JOINFS_JFS_UTIL_H
#define JOINFS_JFS_UTIL_H

#include <sys/types.h>

/*
 * Get the inode number associated with a path.
 */
int jfs_util_get_inode(const char *path);

/*
 * Returns both the inode and mode for the path.
 */
int jfs_util_get_inode_and_mode(const char *path, int *inode, mode_t *mode);

int jfs_util_is_realpath(const char *path);

int jfs_util_is_path_dynamic(const char *path);

/*
 * Get the datapath associated with a path.
 */
int jfs_util_get_datapath(const char *path, char **datapath);

/*
 * Get both the datapath and datainode associated with a file.
 */
int jfs_util_get_datapath_and_datainode(const char *path, char **datapath, int *datainode);

/*
 * Retrieve the datainode associated with a path.
 */
int jfs_util_get_datainode(const char *path);

/* 
 * Returns a pointer to the filename portion of a path.
 */
char *jfs_util_get_filename(const char *path);

int jfs_util_get_subpath(const char *path, char **subpath);

int jfs_util_change_filename(const char *path, const char *filename, char **newpath);

/*
 * Returns the id for the specified key.
 * 
 * If the key doesn't exist, it is added
 * and the new id is returned.
 */
int jfs_util_get_keyid(const char *key);

/*
 * Return the datainode associated with a path.
 *
 * Returns -1 on error.
 */
int jfs_util_get_datainode(const char *path);

int jfs_util_file_cache_failure(int syminode, char **datapath, int *datainode);

int jfs_util_file_cache_sympath_failure(int datainode, char **spath);

int jfs_util_strip_last_path_item(char *path);

char *jfs_util_get_last_path_item(const char *path);

#endif
