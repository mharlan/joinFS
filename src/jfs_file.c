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
#include "jfs_uuid.h"
#include "jfs_file.h"
#include "jfs_util.h"
#include "jfs_file_cache.h"
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
static int jfs_file_do_create(const char *path, mode_t mode);
static int jfs_file_do_mknod(const char *path, mode_t mode);
static int jfs_file_do_rename(const char *from, const char *to);
static int jfs_file_db_add(const char *path, int syminode, int datainode, const char *datapath, const char *filename, int mode);

/*
 * Create a joinFS static file. The file is added
 * to the Linux VFS and the database.
 *
 * Returns the inode of the newly created file or -1.
 */
int
jfs_file_create(const char *path, mode_t mode)
{
  char *realpath;

  int datainode;
  int rc;

  if(jfs_util_is_path_dynamic(path)) {
    datainode = jfs_util_get_datainode(path);
    if(datainode < 0) {
      return datainode;
    }

    rc = jfs_file_cache_get_sympath(datainode, &realpath);
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

  rc = jfs_file_do_create(realpath, mode);
  free(realpath);

  if(rc) {
    return rc;
  }

  return 0;
}

static int
jfs_file_do_create(const char *path, mode_t mode)
{
  char *datapath;
  char *filename;

  int datainode;
  int syminode;
  int rc;
  int fd;

  rc = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
  if(rc < 0) {
	return -errno;
  }

  rc = close(rc);
  if(rc) {
	return -errno;
  }

  syminode = jfs_util_get_inode(path);
  if(syminode < 0) {
	return syminode;
  }

  rc = jfs_uuid_new_datapath(&datapath);
  if(rc) {
	return rc;
  }

  fd = creat(datapath, mode);
  if(fd < 0) {
	free(datapath);
	return -errno;
  }

  filename = jfs_util_get_filename(path);

  datainode = jfs_util_get_inode(datapath);
  if(datainode < 0) {
	close(fd);
	free(datapath);
	return datainode;
  }

  rc = jfs_file_db_add(path, syminode, datainode, datapath, filename, mode);
  if(rc) {
	log_error("New file database inserts failed.\n");
  }
  free(datapath);

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
  
  int datainode;
  int rc;

  if(jfs_util_is_path_dynamic(path)) {
    datainode = jfs_util_get_datainode(path);
    if(datainode < 0) {
      return datainode;
    }

    rc = jfs_file_cache_get_sympath(datainode, &realpath);
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
	rc = jfs_file_do_mknod(realpath, mode);
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

  if(rc) {
    return rc;
  }

  return 0;
}

static int
jfs_file_do_mknod(const char *path, mode_t mode)
{
  char *datapath;
  char *filename;

  int datainode;
  int syminode;
  int rc;

  rc = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
  if(rc < 0) {
	return -errno;
  }

  rc = close(rc);
  if(rc) {
	return -errno;
  }

  syminode = jfs_util_get_inode(path);
  if(syminode < 0) {
	return syminode;
  }

  rc = jfs_uuid_new_datapath(&datapath);
  if(rc) {
	return rc;
  }

  rc = open(datapath, O_CREAT | O_EXCL | O_WRONLY, mode);
  if(rc < 0) {
	free(datapath);
	return -errno;
  }

  rc = close(rc);
  if(rc) {
    free(datapath);
	return -errno;
  }

  filename = jfs_util_get_filename(path);

  datainode = jfs_util_get_inode(datapath);
  if(datainode < 0) {
	free(datapath);
	return datainode;
  }
  
  rc = jfs_file_db_add(path, syminode, datainode, datapath, filename, mode);
  if(rc < 0) {
	log_error("New file database inserts failed.\n");
  }
  free(datapath);

  return rc;
}

/*
 * Used by jfs_file_create and jfs_file_mknod to add
 * the file to the database.
 *
 * It is two db_ops because I can't compile two statements 
 * into a single sqlite3_stmt
 */
static int
jfs_file_db_add(const char *path, int syminode, int datainode, const char *datapath, const char *filename, int mode)
{
  struct jfs_db_op *db_op;

  int rc;

  /* first add to the files table */
  rc = jfs_db_op_create(&db_op, jfs_write_op, 
                        "INSERT OR ROLLBACK INTO files VALUES(%d,\"%s\",\"%s\");",
                        datainode, datapath, filename);
  if(rc) {
	return rc;
  }
  
  jfs_write_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }
  jfs_db_op_destroy(db_op);

  /* now add to symlinks */
  rc = jfs_db_op_create(&db_op, jfs_write_op, 
                        "INSERT OR ROLLBACK INTO symlinks VALUES(%d,\"%s\",%d);",
                        syminode, path, datainode);
  if(rc) {
	return rc;
  }
  
  jfs_write_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }
  jfs_db_op_destroy(db_op);

  rc = jfs_file_cache_add(syminode, path, datainode, datapath);
  if(rc) {
    log_error("Failed to insert new file into file cache.\n");
    return rc;
  }

  return 0;
}

/*
 * Deletes a joinFS static file.
 */
int
jfs_file_unlink(const char *path)
{
  char *datapath;
  char *sympath;

  int datainode;
  int rc;
 
  //see if unlink was called on a dynamic path object
  if(!jfs_util_is_realpath(path)) {
    rc = jfs_dynamic_path_resolution(path, &datapath, &datainode);
    if(rc) {
      return rc;
    }

    if(strcmp(jfs_util_get_filename(datapath), ".jfs_sub_query") == 0) {
      free(datapath);
      return -EISDIR;
    }
    free(datapath);

    rc = jfs_file_cache_get_sympath(datainode, &sympath);
    if(rc) {
      return rc;
    }

    rc = jfs_dynamic_hierarchy_unlink(path);
    if(rc) {
      free(sympath);
      return rc;
    }

    rc = jfs_file_do_unlink(sympath);
    if(rc) {
      log_error("Hardlink unlink failed.\n");
    }
    free(sympath);

    return 0;
  }

  return jfs_file_do_unlink(path);
}

static int
jfs_file_do_unlink(const char *path)
{
  struct jfs_db_op *db_op;
  
  char *datapath;

  int datainode;
  int inode;
  int rc;

  rc = jfs_util_get_datapath_and_datainode(path, &datapath, &datainode);
  if(rc) {
    return rc;
  }

  inode = jfs_util_get_inode(path);
  if(inode < 0) {
    free(datapath);
    return inode;
  }
  
  rc = unlink(path);
  if(rc) {
    free(datapath);
    return -errno;
  }
  
  if(datainode > 0) {
    rc = jfs_db_op_create(&db_op, jfs_write_op, 
                          "DELETE FROM files WHERE inode=%d;",
                          datainode);
    if(rc) {
      free(datapath);
      return rc;
    }
    
    jfs_write_pool_queue(db_op);
    
    rc = jfs_db_op_wait(db_op);
    if(rc) {
      jfs_db_op_destroy(db_op);
      free(datapath);
      return rc;
    }
    jfs_db_op_destroy(db_op);
    
    rc = jfs_db_op_create(&db_op, jfs_write_op,
                          "DELETE FROM symlinks WHERE datainode=%d;",
                          datainode);
    if(rc) {
      free(datapath);
      return rc;
    }
    
    jfs_write_pool_queue(db_op);
    
    rc = jfs_db_op_wait(db_op);
    if(rc) {
      jfs_db_op_destroy(db_op);
      free(datapath);
      return rc;
    }
    jfs_db_op_destroy(db_op);
    
    rc = jfs_db_op_create(&db_op, jfs_write_op,
                          "DELETE FROM metadata WHERE inode=%d;",
                          datainode);
    if(rc) {
      free(datapath);
      return rc;
    }
    
    jfs_write_pool_queue(db_op);
    
    rc = jfs_db_op_wait(db_op);
    if(rc) {
      jfs_db_op_destroy(db_op);
      free(datapath);
      return rc;
    }
    jfs_db_op_destroy(db_op);
  }
  
  rc = unlink(datapath);
  if(rc) {
    rc = -errno;
  }
  free(datapath);
  
  rc = jfs_file_cache_remove(inode);
  if(rc) {
    return rc;
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
  char *subpath;
  char *filename;
  char *from_datapath;
  char *to_datapath;
  char *real_from;
  char *real_to;

  size_t real_len;

  int from_is_dynamic;
  int to_is_dynamic;
  int from_datainode;
  int to_datainode;
  int rc;

  to_is_dynamic = 0;
  from_is_dynamic = 0;
  subpath = NULL;
  from_subpath = NULL;
  to_subpath = NULL;
  from_datapath = NULL;
  to_datapath = NULL;
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
    rc = jfs_dynamic_path_resolution(from, &from_datapath, &from_datainode);

    //.jfs_sub_query edge case
    if(rc) {
      rc = jfs_dynamic_path_resolution(from_subpath, &from_datapath, &from_datainode);
      if(rc) {
        goto cleanup;
      }
      
      real_len = strlen(from_datapath) + strlen(filename) + 2; //null and '/'
      real_from = malloc(sizeof(*real_from) * real_len);
      if(!real_from) {
        rc = -ENOMEM;

        goto cleanup;
      }
      snprintf(real_from, real_len, "%s/%s", from_datapath, filename);

      if(!jfs_util_is_realpath(real_from)) {
        rc = -ENOENT;

        goto cleanup;
      }
    }
    else {
      //can't rename a dynamic folder
      if(strcmp(jfs_util_get_filename(from_datapath), ".jfs_sub_query") == 0) {
        rc = -EISDIR;

        goto cleanup;
      }
      from_is_dynamic = 1;

      rc = jfs_file_cache_get_sympath(from_datainode, &real_from);
      if(rc) {
        goto cleanup;
      }
    }
  }
  else {
    rc = jfs_util_get_datapath_and_datainode(from, &from_datapath, &from_datainode);
    if(rc) { 
      goto cleanup;
    }
    
    real_from = (char *)from;
  }

  filename = jfs_util_get_filename(to);
  if(!filename) {
    rc = -ENOENT;
    
    goto cleanup;
  }

  //is too a dynamic path item
  if(jfs_util_is_path_dynamic(to)) {
    rc = jfs_dynamic_path_resolution(to, &to_datapath, &to_datainode);
    
    //.jfs_sub_query edge case
    if(rc) {
      rc = jfs_dynamic_path_resolution(to_subpath, &to_datapath, &to_datainode);
      if(rc) {
        goto cleanup;
      }
      
      real_len = strlen(to_datapath) + strlen(filename) + 2; //null and '/'
      real_to = malloc(sizeof(*real_to) * real_len);
      if(!real_to) {
        rc = -ENOMEM;

        goto cleanup;
      }
      snprintf(real_to, real_len, "%s/%s", to_datapath, filename);
      
      if(!jfs_util_is_realpath(real_to)) {
        rc = -ENOENT;

        goto cleanup;
      }
    }
    else {
      //can't rename a dynamic folder
      if(strcmp(jfs_util_get_filename(to_datapath), ".jfs_sub_query") == 0) {
        rc = -EISDIR;

        goto cleanup;
      }
      
      rc = jfs_file_cache_get_sympath(to_datainode, &real_to);
      if(rc) {
        goto cleanup;
      }
      
      rc = jfs_util_get_datapath_and_datainode(from, &to_datapath, &to_datainode);
      if(rc) {
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
    free(subpath);
    
    to_is_dynamic = 1;
  }
  else {
    real_to = (char *)to;
  }
  
  rc = jfs_file_do_rename(real_from, real_to);

  if(rc) {
    log_error("Rename failed, rc:%d\n", rc);
    goto cleanup;
  }

  printf("from_is_dynamic:%d, to_is_dynamic:%d\n", from_is_dynamic, to_is_dynamic);

  if(from_is_dynamic) {
    rc = jfs_dynamic_hierarchy_unlink(from);
    if(rc) {
      goto cleanup;
    }
  }

  if(to_is_dynamic) {
    rc = jfs_dynamic_hierarchy_unlink(to);
    if(rc && rc != -ENOENT) {
      log_error("Hierarchy Unlink Error:%d\n", rc);

      goto cleanup;
    }

    rc = jfs_dynamic_hierarchy_add_file(to, from_datapath, from_datainode);
    if(rc) {
      log_error("Hierarchy Add Error:%d\n", rc);
    }
  }
  
cleanup:
  free(subpath);
  free(from_subpath);
  free(to_subpath);
  free(from_datapath);
  free(to_datapath);

  if(real_from != from) {
    free(real_from);
  }
  if(real_to != to) {
    free(real_to);
  }

  return rc;
}

static int
jfs_file_do_rename(const char *from, const char *to)
{
  struct jfs_db_op *db_op;

  char *from_copy;
  char *to_copy;
  char *to_datapath;
  char *filename;

  size_t from_len;
  size_t to_len;

  mode_t from_mode;
  mode_t to_mode;

  int from_datainode;
  int to_datainode;
  int from_inode;
  int to_inode;
  int rc;

  from_copy = NULL;
  to_copy = NULL;
  to_datapath = NULL;

  from_len = strlen(from) + 1;
  from_copy = malloc(sizeof(*from_copy) * from_len);
  if(!from_copy) {
    rc = -ENOMEM;

    goto cleanup;
  }
  strncpy(from_copy, from, from_len);

  to_len = strlen(to) + 1;
  to_copy = malloc(sizeof(*to_copy) * to_len);
  if(!to_copy) {
    rc = -ENOMEM;

    goto cleanup;
  }
  strncpy(to_copy, to, to_len);

  rc = jfs_util_get_inode_and_mode(from, &from_inode, &from_mode);
  if(rc) {
    goto cleanup;
  }
  from_datainode = jfs_util_get_datainode(from);

  //see if to exists
  to_inode = -1;
  rc = jfs_util_get_inode_and_mode(to, &to_inode, &to_mode);
  if(!rc) {
    rc = jfs_util_get_datapath_and_datainode(to, &to_datapath, &to_datainode);
    if(rc) {
      goto cleanup;
    }
  }

  //perform the rename
  rc = rename(from_copy, to_copy);
  if(rc) {
    rc = -errno;

    goto cleanup;
  }

  if(to_inode > -1) {
    //preserve the old metadata
    rc = jfs_db_op_create(&db_op, jfs_write_op,
                          "UPDATE OR ROLLBACK metadata SET inode=%d WHERE inode=%d;",
                          from_datainode, to_datainode);
    if(rc) {
      goto cleanup;
    }
    
    jfs_write_pool_queue(db_op);
    
	rc = jfs_db_op_wait(db_op);
	jfs_db_op_destroy(db_op);

    if(rc) {
      goto cleanup;
    }

    //delete datapath for to
    rc = unlink(to_datapath);
    if(rc) {
      rc = -errno;

      goto cleanup;
    }

    //remove from the datapath cache
    rc = jfs_datapath_cache_remove(to_datainode);
    if(rc) {
      goto cleanup;
    }
  }
  
  if(S_ISREG(from_mode)) {
    filename = jfs_util_get_filename(to);
  
    //update the hardlink
    rc = jfs_db_op_create(&db_op, jfs_write_op,
                          "UPDATE OR ROLLBACK symlinks SET sympath=\"%s\", datainode=%d WHERE syminode=%d;",
                          to, from_datainode, from_inode);
    if(rc) {
      goto cleanup;
    }
    
    jfs_write_pool_queue(db_op);
    
    rc = jfs_db_op_wait(db_op);
    jfs_db_op_destroy(db_op);

    if(rc) {
      goto cleanup;
    }

    //change the filename
    rc = jfs_db_op_create(&db_op, jfs_write_op,
                          "UPDATE OR ROLLBACK files SET filename=\"%s\" WHERE inode=(SELECT datainode FROM symlinks WHERE syminode=%d);",
                          filename, from_inode);
    if(rc) {
      goto cleanup;
    }
    
    jfs_write_pool_queue(db_op);
    
    rc = jfs_db_op_wait(db_op);
    jfs_db_op_destroy(db_op);

    if(rc) {
      goto cleanup;
    }
    
    rc = jfs_file_cache_update_sympath(from_inode, to);
    if(rc) {
      goto cleanup;
    }
  }
  else if(S_ISDIR(from_mode) && from_inode != to_inode) {
    rc = jfs_db_op_create(&db_op, jfs_write_op,
                          "UPDATE OR ROLLBACK files SET inode=%d WHERE inode=%d;",
                          from_inode, to_inode);
    if(rc) {
      goto cleanup;
    }
    
    jfs_write_pool_queue(db_op);
    
    rc = jfs_db_op_wait(db_op);
    jfs_db_op_destroy(db_op);

    if(rc) {
      goto cleanup;
    }
  }

  if(to_inode) {
    //cleanup the old datapath in the db
    rc = jfs_db_op_create(&db_op, jfs_write_op,
                          "DELETE FROM files WHERE inode=%d;",
                          to_datainode);
    if(rc) {
      goto cleanup;
    }
    
    jfs_write_pool_queue(db_op);
    
    rc = jfs_db_op_wait(db_op);
    jfs_db_op_destroy(db_op);

    if(rc) {
      goto cleanup;
    }
  }

 cleanup:
  free(from_copy);
  free(to_copy);
  free(to_datapath);
  
  return rc;
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

  int datainode;
  int rc;

  if(jfs_util_is_path_dynamic(path)) {
    datainode = jfs_util_get_datainode(path);
    if(datainode < 0) {
      return datainode;
    }

    rc = jfs_file_cache_get_sympath(datainode, &realpath);
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
  if(rc) {
    log_error("Failed to open file at:%s, flags:%d\n", realpath, flags);
  }
  free(realpath);

  return rc;
}

static int
jfs_file_do_open(const char *path, int flags)
{
  char *datapath;
  int fd;
  int rc;

  if(flags & O_CREAT) {
	rc = open(path, O_CREAT | O_EXCL | O_WRONLY);
    
	if(rc > 0) {
	  close(rc);

	  rc = unlink(path);
	  if(rc) {
		return -errno;
	  }
      
	  return jfs_file_do_create(path, flags);
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

  rc = jfs_util_get_datapath(path, &datapath);
  if(rc) {
	return rc;
  }

  fd = open(datapath, flags);
  free(datapath);

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
  
  rc = stat(datapath, stbuf);
  free(datapath);

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

/*
  This code does not look right.
 */
int 
jfs_file_readlink(const char *path, char *buf, size_t size)
{
  char *datapath;

  int rc;

  rc = jfs_util_get_datapath(path, &datapath);
  if(rc) {
    return rc;
  }

  rc = readlink(datapath, buf, size - 1);
  free(datapath);

  if(rc) {
    return -errno;
  }
  
  return 0;
}

int 
jfs_file_symlink(const char *from, const char *to)
{
  char *datapath_from;
  char *realpath_to;

  int rc;

  rc = jfs_util_get_datapath(from, &datapath_from);
  if(rc) {
    return rc;
  }

  rc = jfs_util_resolve_new_path(to, &realpath_to);
  if(rc) {
    free(datapath_from);
    return rc;
  }

  rc = symlink(datapath_from, realpath_to);
  free(realpath_to);
  free(datapath_from);

  if(rc) {
    return -errno;
  }

  return 0;
}

int 
jfs_file_link(const char *from, const char *to)
{
  char *datapath_from;
  char *realpath_to;

  int rc;
  
  rc = jfs_util_get_datapath(from, &datapath_from);
  if(rc) {
    return rc;
  }

  rc = jfs_util_resolve_new_path(to, &realpath_to);
  if(rc) {
    free(datapath_from);
    return rc;
  }

  rc = link(datapath_from, realpath_to);
  free(realpath_to);
  free(datapath_from);

  if(rc) {
    return -errno;
  }

  return 0;
}
