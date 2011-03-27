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

#if !defined(_REENTRANT)
#define	_REENTRANT
#endif

#include "error_log.h"
#include "jfs_file.h"
#include "jfs_util.h"
#include "jfs_dynamic_paths.h"
#include "jfs_datapath_cache.h"
#include "sqlitedb.h"
#include "joinfs.h"

#include <fuse.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

static int jfs_file_do_open(const char *path, int flags);
static int jfs_file_do_unlink(const char *path);
static int jfs_file_do_create(const char *path, int flags, mode_t mode);
static int jfs_file_do_rename(const char *from, const char *to);

/*
 * Create a joinFS static file. The file is added
 * to the Linux VFS and the database.
 *
 * Returns the inode of the newly created file or -1.
 */
int
jfs_file_create(const char *path, int flags, mode_t mode)
{
  char *realpath;
  
  int rc;

  if(jfs_util_is_path_dynamic(path)) {
    rc = jfs_util_get_datapath(path, &realpath);
    if(rc) {
      return rc;
    }
  }
  else {
    rc = jfs_util_resolve_new_path(path, &realpath);
    if(rc) {
      return rc;
    }
  }

  rc = jfs_file_do_create(realpath, flags, mode);
  free(realpath);

  if(rc) {
    return rc;
  }

  return 0;
}

static int
jfs_file_do_create(const char *path, int flags, mode_t mode)
{
  char *filename;

  int inode;
  int rc;
  int fd;
  
  fd = open(path, flags, mode);
  if(fd < 0) {
	return -errno;
  }

  filename = jfs_util_get_filename(path);

  inode = jfs_util_get_inode(path);
  if(inode < 0) {
	close(fd);
	return inode;
  }

  rc = jfs_file_db_add(inode, path, filename);
  if(rc) { 
    close(fd);
    return rc;
  }
  
  return fd;
}

/*
 * Create a joinFS static file. The file is added
 * to the Linux VFS and the database.
 *
 * Returns 0 or -1 if there was an error.
 */
int
jfs_file_mknod(const char *path, mode_t mode, dev_t rdev)
{
  char *realpath;
  
  int rc;

  if(jfs_util_is_path_dynamic(path)) {
    rc = jfs_util_get_datapath(path, &realpath);
    if(rc) {
      return rc;
    }
  }
  else {
    rc = jfs_util_resolve_new_path(path, &realpath);
    if(rc) {
      return rc;
    }
  }
  
  if(S_ISREG(mode)) {
	rc = jfs_file_do_create(realpath, O_CREAT | O_EXCL | O_WRONLY, mode);
  } 
  else if(S_ISFIFO(mode)) {
    rc = mkfifo(realpath, mode);
    if(rc) {
      rc = -errno;
    }
  }
  else {
    rc = mknod(realpath, mode, rdev);
    if(rc) {
      rc = -errno;
    }
  }
  free(realpath);
  
  return rc;
}

/*
 * Add a link to the database.
 */
int
jfs_file_db_add(int inode, const char *path, const char *filename)
{
  struct jfs_db_op *db_op;

  int rc;

  /* first add to the files table */
  rc = jfs_db_op_create(&db_op, jfs_write_op,
                        "INSERT OR ROLLBACK INTO links VALUES(NULL, %d,\"%s\",\"%s\");",
                        inode, path, filename);
  if(rc) {
	return rc;
  }
  
  jfs_write_pool_queue(db_op);
  rc = jfs_db_op_wait(db_op);
  jfs_db_op_destroy(db_op);

  if(rc) {
	return rc;
  }
  
  return 0;
}

/*
 * Deletes a hardlink or symlink.
 */
int
jfs_file_unlink(const char *path)
{
  char *datapath;

  int jfs_id;
  int rc;
 
  //see if unlink was called on a dynamic path object
  if(!jfs_util_is_realpath(path)) {
    rc = jfs_dynamic_path_resolution(path, &datapath, &jfs_id);
    if(rc) {
      return rc;
    }

    if(strcmp(jfs_util_get_filename(datapath), ".jfs_sub_query") == 0) {
      free(datapath);
      return -EISDIR;
    }

    rc = jfs_dynamic_hierarchy_unlink(path);
    if(rc) {
      free(datapath);
      return rc;
    }

    rc = jfs_file_do_unlink(datapath);
    free(datapath);

    return rc;
  }

  return jfs_file_do_unlink(path);
}

static int
jfs_file_do_unlink(const char *path)
{
  struct jfs_db_op *db_op;
  
  char *link_query;
  char *metadata_query;
  
  int rc;

  rc = jfs_db_op_create_query(&metadata_query,
                              "DELETE FROM metadata WHERE jfs_id=(SELECT jfs_id FROM links WHERE path=\"%s\");",
                              path);
  if(rc) {
    return rc;
  }

  rc = jfs_db_op_create_query(&link_query, 
                              "DELETE FROM links WHERE path=\"%s\";",
                              path);
  if(rc) {
    free(metadata_query);
    return rc;
  }
  
  rc = jfs_db_op_create_multi_op(&db_op, 2, metadata_query, link_query);
  if(rc) {
    free(metadata_query);
    free(link_query);
    
    return rc;
  }
  
  jfs_write_pool_queue(db_op);
  rc = jfs_db_op_wait(db_op);
  jfs_db_op_destroy(db_op);
  
  if(rc) {
    return rc;
  }

  rc = unlink(path);
  if(rc) {
    return -errno;
  }

  return 0;
}

/*
 * Perform a rename on a joinFS file.
 */
int
jfs_file_rename(const char *from, const char *to)
{
  char *from_subpath;
  char *to_subpath;
  char *from_datapath;
  char *to_datapath;
  char *subpath;
  char *filename;
  char *real_from;
  char *real_to;

  size_t real_len;

  int from_is_dynamic;
  int to_is_dynamic;
  int from_id;
  int to_id;
  int rc;

  from_id = 0;
  to_id = 0;
  to_is_dynamic = 0;
  from_is_dynamic = 0;
  from_datapath = NULL;
  to_datapath = NULL;
  subpath = NULL;
  from_subpath = NULL;
  to_subpath = NULL;
  real_from = NULL;
  real_to = NULL;

  filename = jfs_util_get_filename(from);
  if(!filename) {
    rc = -ENOENT;

    goto cleanup;
  }

  rc = jfs_util_get_subpath(from, &from_subpath);
  if(rc) {
    goto cleanup;
  }

  rc = jfs_util_get_subpath(to, &to_subpath);
  if(rc) {
    goto cleanup;
  }

  //see if rename was called from a dynamic object
  if(!jfs_util_is_realpath(from)) {
    rc = jfs_dynamic_path_resolution(from, &real_from, &from_id);

    //.jfs_sub_query edge case
    if(rc) {
      rc = jfs_dynamic_path_resolution(from_subpath, &from_datapath, &from_id);
      if(rc) {
        goto cleanup;
      }
      
      real_len = strlen(from_datapath) + strlen(filename) + 2; //null and '/'
      real_from = malloc(sizeof(*real_from) * real_len);
      if(!real_from) {
        free(from_datapath);
        rc = -ENOMEM;

        goto cleanup;
      }
      snprintf(real_from, real_len, "%s/%s", from_datapath, filename);
      free(from_datapath);

      if(!jfs_util_is_realpath(real_from)) {
        rc = -ENOENT;

        goto cleanup;
      }
    }
    else {
      //can't rename a dynamic folder
      if(strcmp(jfs_util_get_filename(real_from), ".jfs_sub_query") == 0) {
        rc = -EISDIR;

        goto cleanup;
      }
      
      from_is_dynamic = 1;
    }
  }
  else {
    real_from = (char *)from;
  }

  filename = jfs_util_get_filename(to);
  if(!filename) {
    rc = -ENOENT;
    
    goto cleanup;
  }

  //is too a dynamic path item
  if(jfs_util_is_path_dynamic(to)) {
    rc = jfs_dynamic_path_resolution(to, &real_to, &to_id);
    
    //.jfs_sub_query edge case
    if(rc) {
      rc = jfs_dynamic_path_resolution(to_subpath, &to_datapath, &to_id);
      if(rc) {
        goto cleanup;
      }
      
      real_len = strlen(to_datapath) + strlen(filename) + 2; //null and '/'
      real_to = malloc(sizeof(*real_to) * real_len);
      if(!real_to) {
        free(to_datapath);
        rc = -ENOMEM;

        goto cleanup;
      }
      snprintf(real_to, real_len, "%s/%s", to_datapath, filename);
      free(to_datapath);
      
      if(!jfs_util_is_realpath(real_to)) {
        rc = -ENOENT;

        goto cleanup;
      }
    }
    else {
      //can't rename a dynamic folder
      if(strcmp(jfs_util_get_filename(real_to), ".jfs_sub_query") == 0) {
        rc = -EISDIR;

        goto cleanup;
      }
      
      to_is_dynamic = 1;
    }
  }
  //is to being saved to a dynamic folder?
  else if(from_is_dynamic && (strcmp(to_subpath, from_subpath) == 0)) {
    rc = jfs_util_get_subpath(real_from, &subpath);
    if(rc) {
      goto cleanup;
    }
    
    real_len = strlen(subpath) + strlen(filename) + 1;
    real_to = malloc(sizeof(*real_to) * real_len);
    if(!real_to) {
      rc = -ENOMEM;

      goto cleanup;
    }
    snprintf(real_to, real_len, "%s%s", subpath, filename);
    
    to_is_dynamic = 1;
  }
  else {
    real_to = (char *)to;
  }
  
  rc = jfs_file_do_rename(real_from, real_to);

  if(rc) {
    goto cleanup;
  }
  
  if(from_is_dynamic) {
    rc = jfs_dynamic_hierarchy_unlink(from);
    if(rc) {
      goto cleanup;
    }
  }

  if(to_is_dynamic) {
    rc = jfs_dynamic_hierarchy_unlink(to);
    if(rc && rc != -ENOENT) {
      goto cleanup;
    }

    rc = jfs_dynamic_hierarchy_add_file(to, real_from, from_id);
  }
  
cleanup:
  if(subpath) {
    free(subpath);
  }
  if(from_subpath) {
    free(from_subpath);
  }
  if(to_subpath) {
    free(to_subpath);
  }
  if(real_from != from && real_from) {
    free(real_from);
  }
  if(real_to != to && real_to) {
    free(real_to);
  }

  return rc;
}

static int
jfs_file_do_rename(const char *from, const char *to)
{
  struct jfs_db_op *db_op;
  
  char *filename;

  char *metadata_query;
  char *link_query;
  char *link_delete_query;
  
  int to_exists;
  int rc;

  metadata_query = NULL;
  link_query = NULL;
  link_delete_query = NULL;
  
  if(jfs_util_is_realpath(to)) {
    to_exists = 1;
  }
  else {
    to_exists = 0;
  }
  
  filename = jfs_util_get_filename(to);
  
  //update the hardlink
  rc = jfs_db_op_create_query(&link_query,
                              "UPDATE links SET path=\"%s\", filename=\"%s\" WHERE path=\"%s\";",
                              to, filename, from);
  if(rc) {
    return rc;
  }

  if(to_exists) {
    //preserve the old metadata
    rc = jfs_db_op_create_query(&metadata_query,
                                "UPDATE metadata SET jfs_id=(SELECT jfs_id FROM links WHERE path=\"%s\") WHERE jfs_id=(SELECT jfs_id FROM links WHERE path=\"%s\");",
                                from, to); 
    if(rc) {
      free(link_query);
      
      return rc;
    }
    
    //cleanup the old datapath in the db
    rc = jfs_db_op_create_query(&link_delete_query,
                                "DELETE FROM links WHERE path=\"%s\";",
                                to);
    if(rc) {
      free(link_query);
      free(metadata_query);
      
      return rc;
    }
    
    rc = jfs_db_op_create_multi_op(&db_op, 3, metadata_query, link_delete_query,
                                   link_query);
    if(rc) {
      free(metadata_query);
      free(link_delete_query);
      free(link_query);
      
      return rc;
    }
  }
  else {
    rc = jfs_db_op_create_multi_op(&db_op, 1, link_query);
    if(rc) {
      free(link_query);

      return rc;
    }
  }
  
  jfs_write_pool_queue(db_op);
  rc = jfs_db_op_wait(db_op);
  jfs_db_op_destroy(db_op);
  
  if(rc) {
    return rc;
  }
  
  //perform the rename
  rc = rename(from, to);
  if(rc) {
    return -errno;
  }

  return 0;
}

/*
 * Truncate a joinFS file.
 */
int
jfs_file_truncate(const char *path, off_t size)
{
  char *datapath;
  int rc;

  rc = jfs_util_get_datapath(path, &datapath);
  if(rc) {
	return rc;
  }

  rc = truncate(datapath, size);
  if(rc) {
	rc = -errno;
  }
  free(datapath);

  return rc;
}

/*
 * Open a joinFS file.
 */
int
jfs_file_open(const char *path, int flags)
{
  char *realpath;
  
  int rc;

  if(jfs_util_is_path_dynamic(path)) {
    rc = jfs_util_get_datapath(path, &realpath);
    if(rc) {
      return rc;
    }
  }
  else {
    rc = jfs_util_resolve_new_path(path, &realpath);
    if(rc) {
      return rc;
    }
  }

  rc = jfs_file_do_open(realpath, flags);
  free(realpath);

  return rc;
}

static int
jfs_file_do_open(const char *path, int flags)
{
  int fd;
  int rc;

  if(flags & O_CREAT) {
	fd = open(path, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR);
    
	if(fd > 0) {
	  close(fd);

	  rc = unlink(path);
	  if(rc) {
		return -errno;
	  }
      
	  return jfs_file_do_create(path, flags, S_IRUSR);
	}
	else if(errno == EEXIST) {
	  if(flags & O_EXCL) {
		return -errno;
	  }
	}
	else {
	  return -errno;
	}
  }
  
  fd = open(path, flags);
  if(fd < 0) {
	return -errno;
  }

  return fd;
}

/*
 * Get the system attributes for a file.
 */
int
jfs_file_getattr(const char *path, struct stat *stbuf)
{
  char *datapath;

  int rc;
  
  rc = jfs_util_get_datapath(path, &datapath);
  if(rc) {
    return rc;
  }
  
  rc = lstat(datapath, stbuf);
  if(datapath) {
    free(datapath);
  }

  if(rc) {
	return -errno;
  }

  return 0;
}

int
jfs_file_utimes(const char *path, const struct timeval tv[2])
{
  char *datapath;
  int rc;
  
  rc = jfs_util_get_datapath(path, &datapath);
  if(rc) {
	return rc;
  }
 
  rc = utimes(datapath, tv);
  free(datapath);

  if(rc) {
	return -errno;
  }

  return 0;
}

int
jfs_file_statfs(const char *path, struct statvfs *stbuf)
{
  char *datapath;
  
  int rc;

  rc = jfs_util_get_datapath(path, &datapath);
  if(rc) {
    return rc;
  }

  rc = statvfs(datapath, stbuf);
  free(datapath);

  if(rc) {
    return -errno;
  }

  return 0;
}

int 
jfs_file_readlink(const char *path, char *buf, size_t size)
{
  int rc;
  
  rc = readlink(path, buf, size - 1);
  if(rc < 0) {
    return -errno;
  }
  
  return rc;
}

int 
jfs_file_symlink(const char *from, const char *to)
{
  char *realpath_to;
  char *filename;

  int inode;
  int rc;

  rc = jfs_util_resolve_new_path(to, &realpath_to);
  if(rc) {
    return rc;
  }

  rc = symlink(from, realpath_to);  
  if(rc) { 
    free(realpath_to);
    
    return -errno;
  }
  
  filename = jfs_util_get_filename(realpath_to);
  inode = jfs_util_get_inode(realpath_to);
  if(inode < 0) {
    free(realpath_to);
    
    return -errno;
  }

  rc = jfs_file_db_add(inode, realpath_to, filename);
  free(realpath_to);

  if(rc) {
    return rc;
  }

  return 0;
}

int 
jfs_file_link(const char *from, const char *to)
{
  char *realpath_to;
  char *filename;

  int inode;
  int rc;

  rc = jfs_util_resolve_new_path(to, &realpath_to);
  if(rc) {
    return rc;
  }

  rc = link(from, realpath_to);  
  if(rc) { 
    free(realpath_to);
    
    return -errno;
  }
  
  filename = jfs_util_get_filename(realpath_to);
  inode = jfs_util_get_inode(realpath_to);
  if(inode < 0) {
    free(realpath_to);
    
    return inode;
  }

  rc = jfs_file_db_add(inode, realpath_to, filename);
  free(realpath_to);

  if(rc) {
    return rc;
  }

  return 0;
}
