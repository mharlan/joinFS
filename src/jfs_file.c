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

#include "error_log.h"
#include "jfs_uuid.h"
#include "jfs_file.h"
#include "jfs_util.h"
#include "jfs_file_cache.h"
#include "jfs_dynamic_paths.h"
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

static int jfs_file_do_rename(const char *from, const char *to);
static int jfs_file_db_add(const char *path, int syminode, int datainode, char *datapath, char *filename, int mode);
static int create_datapath(char *uuid, char **datapath);

/*
 * Create a joinFS static file. The file is added
 * to the Linux VFS and the database.
 *
 * Returns the inode of the newly created file or -1.
 */
int
jfs_file_create(const char *path, mode_t mode)
{
  char *uuid;
  char *datapath;
  char *filename;

  int datainode;
  int syminode;
  int rc;
  int fd;

  log_error("Called jfs_file_create.\n");
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

  uuid = jfs_create_uuid();
  jfs_generate_uuid(uuid);

  rc = create_datapath(uuid, &datapath);
  if(rc) {
	return rc;
  }

  log_error("--CREATING FILE AT DATAPATH:%s\n", datapath);

  filename = jfs_util_get_filename(path);

  fd = creat(datapath, mode);
  if(fd < 0) {
	free(datapath);
	return -errno;
  }

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

  return fd;
}

/*
 * Create a joinFS static file. The file is added
 * to the Linux VFS and the database.
 *
 * Returns 0 or -1 if there was an error.
 */
int
jfs_file_mknod(const char *path, mode_t mode)
{
  char *uuid;
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

  uuid = jfs_create_uuid();
  jfs_generate_uuid(uuid);

  rc = create_datapath(uuid, &datapath);
  if(rc) {
	return rc;
  }

  log_error("--CREATING FILE AT DATAPATH:%s\n", datapath);

  filename = jfs_util_get_filename(path);

  rc = open(datapath, O_CREAT | O_EXCL | O_WRONLY, mode);
  if(rc < 0) {
	free(datapath);
	return -errno;
  }

  rc = close(rc);
  if(rc) {
	return -errno;
  }

  datainode = jfs_util_get_inode(datapath);
  if(datainode < 0) {
	free(datapath);
	return datainode;
  }
  
  rc = jfs_file_db_add(path, syminode, datainode, datapath, filename, mode);
  if(rc < 0) {
	log_error("New file database inserts failed.\n");
  }

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
jfs_file_db_add(const char *path, int syminode, int datainode, char *datapath, char *filename, int mode)
{
  struct jfs_db_op *db_op;
  int rc;

  /* first add to the files table */
  rc = jfs_db_op_create(&db_op);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_write_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "INSERT OR ROLLBACK INTO files VALUES(%d,\"%s\",\"%s\");",
		   datainode, datapath, filename);
  
  jfs_write_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);
	free(datapath);
	return rc;
  }
  jfs_db_op_destroy(db_op);

  /* now add to symlinks */
  rc = jfs_db_op_create(&db_op);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_write_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "INSERT OR ROLLBACK INTO symlinks VALUES(%d,\"%s\",%d);",
		   syminode, path, datainode);
  
  jfs_write_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);
	free(datapath);
	return rc;
  }
  jfs_db_op_destroy(db_op);

  return jfs_file_cache_add(syminode, path, datainode, datapath);
}

/*
 * Deletes a joinFS static file.
 */
int
jfs_file_unlink(const char *path)
{
  struct jfs_db_op *db_op;

  char *sympath;
  char *datapath;

  int datainode;
  int syminode;
  int rc;

  //see if unlink was called on a dynamic path object
  if(!jfs_util_is_realpath(path)) {
    rc = jfs_dynamic_path_resolution(path, &datapath, &datainode);
    if(rc) {
      return rc;
    }

    rc = jfs_file_cache_get_sympath(datainode, &sympath);
    if(rc) {
      return rc;
    }

    rc = jfs_dynamic_hierarchy_unlink(path);
    if(rc) {
      return rc;
    }

    return jfs_file_unlink(sympath);
  }

  syminode = jfs_util_get_inode(path);
  rc = jfs_util_get_datapath_and_datainode(path, &datapath, &datainode);
  if(rc) {
    return rc;
  }
  
  rc = unlink(path);
  if(rc) {
    return -errno;
  }
  
  if(datainode > 0) {
    rc = jfs_db_op_create(&db_op);
    if(rc) {
      return rc;
    }
    
    db_op->op = jfs_write_op;
    snprintf(db_op->query, JFS_QUERY_MAX,
             "DELETE FROM files WHERE inode=%d;",
             datainode);
    
    jfs_write_pool_queue(db_op);
    
    rc = jfs_db_op_wait(db_op);
    if(rc) {
      jfs_db_op_destroy(db_op);
      log_error("--Unlink Database Error:%d\n", rc);
      return rc;
    }
    jfs_db_op_destroy(db_op);
    
    rc = jfs_db_op_create(&db_op);
    if(rc) {
      return rc;
    }
    
    db_op->op = jfs_write_op;
    snprintf(db_op->query, JFS_QUERY_MAX,
             "DELETE FROM symlinks WHERE datainode=%d;",
             datainode);
    
    jfs_write_pool_queue(db_op);
    
    rc = jfs_db_op_wait(db_op);
    if(rc) {
      jfs_db_op_destroy(db_op);
      return rc;
    }
    jfs_db_op_destroy(db_op);
    
    rc = jfs_db_op_create(&db_op);
    if(rc) {
      return rc;
    }
    
    db_op->op = jfs_write_op;
    snprintf(db_op->query, JFS_QUERY_MAX,
             "DELETE FROM metadata WHERE inode=%d;",
             datainode);
    
    jfs_write_pool_queue(db_op);
    
    rc = jfs_db_op_wait(db_op);
    if(rc) {
      jfs_db_op_destroy(db_op);
      log_error("--Unlink metadata error:%d\n", rc);
      return rc;
    }
    jfs_db_op_destroy(db_op);
  }
  
  rc = unlink(datapath);
  if(rc) {
    return -errno;
  }
  
  return jfs_file_cache_remove(syminode);
}

/*
 * Perform a rename on a joinFS file.
 */
int
jfs_file_rename(const char *from, const char *to)
{
  char *hardlink_to;
  char *sympath;
  char *filename;
  char *datapath;

  int datainode;
  int rc;

  filename = jfs_util_get_filename(to);

  //see if rename was called on a dynamic path object
  if(!jfs_util_is_realpath(from)) {
    rc = jfs_dynamic_path_resolution(from, &datapath, &datainode);
    if(rc) {
      return rc;
    }

    rc = jfs_file_cache_get_sympath(datainode, &sympath);
    if(rc) {
      return rc;
    }

    rc = jfs_util_change_filename(sympath, filename, &hardlink_to);
    if(rc) {
      return rc;
    }

    printf("---DYNAMIC RENAME: from:%s, to:%s\n", sympath, hardlink_to);

    //perform the rename in the dynamic path hierarchy
    rc = jfs_dynamic_hierarchy_rename(from, filename);
    if(rc) {
      return rc;
    }

    //rename in the VFS
    rc = jfs_file_do_rename(sympath, hardlink_to);
    free(hardlink_to);
    if(rc) {
      return rc;
    }

    return 0;
  }

  //normal rename
  return jfs_file_do_rename(from, to);
}

static int
jfs_file_do_rename(const char *from, const char *to)
{
  struct jfs_db_op *db_op;

  char *filename;

  mode_t mode;

  int datainode;
  int new_datainode;
  int new_inode;
  int inode;
  int rc;

  //see if to exists
  new_inode = jfs_util_get_inode(to);
  if(new_inode > -1) {
    datainode = jfs_util_get_datainode(to);
    new_datainode = jfs_util_get_datainode(from);
    
    rc = jfs_db_op_create(&db_op);
    if(rc) {
      return rc;
    }
    
    db_op->op = jfs_write_op;
    snprintf(db_op->query, JFS_QUERY_MAX,
             "UPDATE OR ROLLBACK metadata SET inode=%d WHERE inode=%d;",
             new_datainode, datainode);
    
    jfs_write_pool_queue(db_op);
    
	rc = jfs_db_op_wait(db_op);
	if(rc) {
	  jfs_db_op_destroy(db_op);
	  return rc;
	}
	jfs_db_op_destroy(db_op);
    
    //cleanup the old to
    rc = jfs_file_unlink(to);
    if(rc) {
      printf("--jfs_file_unlink failed:%d\n", rc);

      return rc;
    }
  }
  
  //get info for from
  rc = jfs_util_get_inode_and_mode(from, &inode, &mode);
  if(rc) {
    printf("--jfs_util_get_inode_and_mode failed rc:%d\n", rc);

    return rc;
  }

  //perform the rename
  rc = rename(from, to);
  if(rc) {
    return -errno;
  }
  
  //get the new inode for to
  new_inode = jfs_util_get_inode(to);
  if(new_inode < 0) {
    printf("--jfs_util_get_inode failed for to, rc:%d\n", rc);

    return new_inode;
  }
  
  printf("----Renamed from:%s, inode:%d, to:%s, new_inode%d, mode:00x%x\n",
         from, inode, to, new_inode, mode);
  
  if(S_ISREG(mode)) {
    filename = jfs_util_get_filename(to);
  
    rc = jfs_db_op_create(&db_op);
    if(rc) {
      return rc;
    }
    
    db_op->op = jfs_write_op;
    snprintf(db_op->query, JFS_QUERY_MAX,
             "UPDATE OR ROLLBACK files SET f.filename=\"%s\" WHERE inode=(SELECT datainode FROM symlinks WHERE syminode=%d);",
             filename, inode);
    
    jfs_write_pool_queue(db_op);
    
    rc = jfs_db_op_wait(db_op);
    if(rc) {
      jfs_db_op_destroy(db_op);
      return rc;
    }
    jfs_db_op_destroy(db_op);
    
    rc = jfs_db_op_create(&db_op);
    if(rc) {
      return rc;
    }
    
    db_op->op = jfs_write_op;
    snprintf(db_op->query, JFS_QUERY_MAX,
			 "UPDATE OR ROLLBACK symlinks SET syminode=%d, sympath=\"%s\" WHERE syminode=%d;",
             new_inode, to, inode);
    
    jfs_write_pool_queue(db_op);
    
    rc = jfs_db_op_wait(db_op);
    if(rc) {
      jfs_db_op_destroy(db_op);
      return rc;
    }
    jfs_db_op_destroy(db_op);
    
    rc = jfs_file_cache_update_sympath(inode, to);
    if(rc) {
      return rc;
    }
  }
  else if(S_ISDIR(mode) && new_inode != inode) {
    rc = jfs_db_op_create(&db_op);
    if(rc) {
      return rc;
    }
    
    db_op->op = jfs_write_op;
    snprintf(db_op->query, JFS_QUERY_MAX,
             "UPDATE OR ROLLBACK files SET inode=%d WHERE inode=%d;",
             inode, new_inode);
    
    jfs_write_pool_queue(db_op);
    
    rc = jfs_db_op_wait(db_op);
    if(rc) {
      jfs_db_op_destroy(db_op);
      return 0;
    }
    
    jfs_db_op_destroy(db_op);
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
	return -errno;
  }

  return 0;
}

/*
 * Open a joinFS file.
 */
int
jfs_file_open(const char *path, int flags)
{
  char *datapath;
  int fd;
  int rc;

  if(flags & O_CREAT) {
    log_msg("!@__-------Open called with O_CREATE flag.\n");

	rc = open(path, O_CREAT | O_EXCL | O_WRONLY);
	if(rc > 0) {
	  close(rc);

	  rc = unlink(path);
	  if(rc) {
		return -errno;
	  }

	  log_msg("**(OPEN CALLED: With create, doesn't exist) path:%s, flags:%d\n", path, flags);

	  return jfs_file_create(path, flags);
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

  log_error("--GETTING ATTRS FOR datapath:%s\n", datapath);

  rc = stat(datapath, stbuf);
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
  if(rc) {
	return -errno;
  }

  return 0;
}

/*
 * Get the location where the data will be saved.
 * 
 * joinFS_root_dir/.data/uuid-string
 *
 * The datapath must be freed if not added to the cache.
 */
static int
create_datapath(char *uuid, char **datapath)
{
  char *dpath;
  int path_len;

  path_len = JFS_CONTEXT->datapath_len + JFS_UUID_LEN + 1;
  dpath = malloc(sizeof(*datapath) * path_len);
  if(!dpath) {
	jfs_destroy_uuid(uuid);
	return -ENOMEM;
  }
  snprintf(dpath, path_len, "%s/%s", JFS_CONTEXT->datapath, uuid);
  jfs_destroy_uuid(uuid);
  *datapath = dpath;

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
  if(rc) {
    return -errno;
  }

  return 0;
}
