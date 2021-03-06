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

#define JFS_THREAD_MIN    12
#define JFS_THREAD_MAX    256
#define JFS_THREAD_LINGER 512

#define FUSE_USE_VERSION  27

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include "error_log.h"
#include "jfs_dir.h"
#include "jfs_file.h"
#include "jfs_meta.h"
#include "jfs_security.h"
#include "jfs_datapath_cache.h"
#include "jfs_key_cache.h"
#include "jfs_meta_cache.h"
#include "jfs_dynamic_paths.h"
#include "thr_pool.h"
#include "sqlitedb.h"
#include "joinfs.h"

#include <fuse.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ulockmgr.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/xattr.h>

struct jfs_context joinfs_context;

static thr_pool_t *jfs_read_pool;
static thr_pool_t *jfs_write_pool;

#define ABS_PATH_INC 512

/*
 * Get a joinFS real path.
 *
 * The realpath must be freed.
 */
static char *
jfs_realpath(const char *path)
{
  char *jfs_path;

  size_t path_len;
  
  path_len = strlen(path) + joinfs_context.querypath_len + 2;
  jfs_path = malloc(sizeof(*jfs_path) * path_len);
  if(!jfs_path) {
	return NULL;
  }

  if(path[0] == '/') {
	snprintf(jfs_path, path_len, "%s%s", joinfs_context.querypath, path);
  }
  else if(path == NULL) {
	snprintf(jfs_path, path_len, "%s/", joinfs_context.querypath);
  }
  else {
	snprintf(jfs_path, path_len, "%s/%s", joinfs_context.querypath, path);
  }
  
  return jfs_path;
}

/*
 * Perform a database read operation.
 */
int 
jfs_read_pool_queue(struct jfs_db_op *db_op)
{
  return jfs_pool_queue(jfs_read_pool, db_op);
}

/*
 * Perform a database write operation.
 */
int 
jfs_write_pool_queue(struct jfs_db_op *db_op)
{
  return jfs_pool_queue(jfs_write_pool, db_op);
}

/*
 * Initialize joinFS.
 */
static void * 
jfs_init(struct fuse_conn_info *conn)
{
  struct sched_param param;
  pthread_attr_t wattr;

  log_init();
  log_msg("Starting joinFS. FUSE Major=%d Minor=%d\n",
          conn->proto_major, conn->proto_minor);

  /* initialize caches */
  jfs_dynamic_path_init();
  jfs_datapath_cache_init();
  jfs_key_cache_init();
  jfs_meta_cache_init();
  jfs_init_db();

  jfs_read_pool = jfs_pool_create(JFS_THREAD_MIN, JFS_THREAD_MAX, 
								  JFS_THREAD_LINGER, NULL, 
								  SQLITE_OPEN_READONLY);
  if(!jfs_read_pool) {
	log_error("Failed to allocate READ pool.\n");
    log_destroy();

	exit(EXIT_FAILURE);
  }

  pthread_attr_init(&wattr);
  pthread_attr_setschedpolicy(&wattr, SCHED_RR);
  pthread_attr_getschedparam(&wattr, &param);
  param.sched_priority = 1;
  pthread_attr_setschedparam(&wattr, &param);

  jfs_write_pool = jfs_pool_create(1, 1, JFS_THREAD_LINGER, 
								   &wattr, SQLITE_OPEN_READWRITE);
  if(!jfs_write_pool) {
	log_error("Failed to allocate WRITE pool.\n");
    log_destroy();

	exit(EXIT_FAILURE);
  }
  
  log_msg("joinFS Thread pools started.\n");
  
  return NULL;
}

static void 
jfs_destroy(void *arg)
{
  int rc;

  /* stop all reads */
  jfs_pool_destroy(jfs_read_pool);
  
  /* let writes propogate */
  jfs_pool_wait(jfs_write_pool);
  jfs_pool_destroy(jfs_write_pool);

  rc = sqlite3_shutdown();
  if(rc != SQLITE_OK) {
	log_error("SQLITE shutdown FAILED!!!\n");
  }
  
  jfs_datapath_cache_destroy();
  jfs_key_cache_destroy();
  jfs_meta_cache_destroy();
  jfs_dynamic_hierarchy_destroy();

  free(joinfs_context.querypath);
  free(joinfs_context.mountpath);
  free(joinfs_context.logpath);
  free(joinfs_context.dbpath);

  log_msg("joinFS shutdown completed successfully.\n", arg);
  log_destroy();
}

static int 
jfs_getattr(const char *path, struct stat *stbuf)
{
  char *jfs_path;
  int rc;
  
  jfs_path = jfs_realpath(path);
  rc = jfs_file_getattr(jfs_path, stbuf);
  free(jfs_path);

  if(rc) {
    if(rc != -ENOENT) {
      log_error("jfs_getattr---path:%s, error:%d\n", path, rc);
    }
    return rc;
  }
  
  return 0;
}

static int 
jfs_access(const char *path, int mask)
{
  char *jfs_path;
  int rc;
  
  jfs_path = jfs_realpath(path);
  rc = jfs_security_access(jfs_path, mask);
  free(jfs_path);

  if(rc) {
    if(rc != -EACCES) {
      log_error("jfs_access---path:%s, mask:%d, error:%d\n", path, mask, rc);
    }
    return rc;
  }
  
  return 0;
}

static int 
jfs_readlink(const char *path, char *buf, size_t size)
{
  char *jfs_path;
  int rc;
  
  jfs_path = jfs_realpath(path);
  rc = jfs_file_readlink(jfs_path, buf, size);
  free(jfs_path);

  if(rc) {
    log_error("jfs_readlink---path:%s, error:%d\n", path, rc);
    return rc;
  }
  
  return 0;
}

static int
jfs_opendir(const char *path, struct fuse_file_info *fi)
{
  DIR *dp;

  char *jfs_path;
  int rc;
  
  jfs_path = jfs_realpath(path);
  rc = jfs_dir_opendir(jfs_path, &dp);
  free(jfs_path);

  if(rc) {
    log_error("jfs_opendir---path%s, error:%d\n", path, rc);
    
    return rc;
  }
  fi->fh = (uintptr_t)dp;

  return 0;
}

static int 
jfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			off_t offset, struct fuse_file_info *fi)
{
  DIR *dp;
  
  char *jfs_path;
  int rc;

  (void) offset;

  jfs_path = jfs_realpath(path);
  dp = (DIR *)(uintptr_t)fi->fh;
  rc = jfs_dir_readdir(jfs_path, dp, buf, filler);
  free(jfs_path);

  if(rc) {
    log_error("jfs_readdir---path:%s, error:%d\n", path, rc);
	return rc;
  }

  return 0;
}

static int
jfs_releasedir(const char *path, struct fuse_file_info *fi)
{
  DIR *dp;

  int rc;
  
  dp = (DIR *)(uintptr_t)fi->fh;
  rc = closedir(dp);
  if(rc) {
    log_error("jfs_releasedir---error:%d\n", -errno);

    return -errno;
  }

  return 0;
}

static int
jfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
  char *jfs_path;
  int fd;
  
  jfs_path = jfs_realpath(path);
  fd = jfs_file_open(jfs_path, fi->flags, mode);
  free(jfs_path);

  if(fd < 0) {
	log_error("jfs_create---path:%s, flags:%d, mode:%d, error:%d\n", path, fi->flags, mode, fd);
	return fd;
  }

  fi->fh = fd;
  fi->direct_io = 1;

  return 0;
}

static int
jfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
  char *jfs_path;
  int rc;
  
  jfs_path = jfs_realpath(path);
  rc = jfs_file_mknod(path, mode, rdev);
  free(jfs_path);

  if(rc) {
    log_error("jfs_mknod---path:%s, mode:%d, rdev:%d, error:%d\n", path, mode, rdev, rc);
	return rc;
  }

  return 0;
}

static int 
jfs_mkdir(const char *path, mode_t mode)
{
  char *jfs_path;
  int rc;
  
  jfs_path = jfs_realpath(path);
  rc = jfs_dir_mkdir(jfs_path, mode);
  free(jfs_path);

  if(rc) {
	log_error("jfs_mkdir path:%s, mode:%d, error:%d\n", path, mode, rc);
    return rc;
  }

  return 0;
}

static int
jfs_unlink(const char *path)
{
  char *jfs_path;
  int rc;
  
  jfs_path = jfs_realpath(path);
  rc = jfs_file_unlink(jfs_path);
  if(jfs_path) {
    free(jfs_path);
  }

  if(rc) {
	log_error("jfs_unlink---path:%s, error:%d\n", path, rc);
    return rc;
  }

  return 0;
}

static int
jfs_rmdir(const char *path)
{
  char *jfs_path;
  int rc;
  
  jfs_path = jfs_realpath(path);
  rc = jfs_dir_rmdir(jfs_path);
  if(jfs_path) {
    free(jfs_path);
  }

  if(rc) {
	log_error("jfs_rmdir---path:%s, error:%d\n", path, rc);
    return rc;
  }

  return 0;
}

static int 
jfs_symlink(const char *from, const char *to)
{
  //char *jfs_path_from;
  char *jfs_path_to;
  
  int rc;
  
  jfs_path_to = jfs_realpath(to);
  rc = jfs_file_symlink(from, jfs_path_to);

  if(jfs_path_to) {
    free(jfs_path_to);
  }

  if(rc) {
    log_error("jfs_symlink---from:%s, to:%s, error:%d\n", from, to, rc);
    return rc;
  }

  return 0;
}

static int 
jfs_rename(const char *from, const char *to)
{
  char *jfs_path_from;
  char *jfs_path_to;

  int rc;

  jfs_path_from = jfs_realpath(from);
  jfs_path_to = jfs_realpath(to);
  rc = jfs_file_rename(jfs_path_from, jfs_path_to);
  
  if(jfs_path_from) {
    free(jfs_path_from);
  }
  if(jfs_path_to) {
    free(jfs_path_to);
  }

  if(rc) {
	log_error("jfs_rename---from:%s, to:%s, error:%d\n", from, to, rc);
    return rc;
  }

  return 0;
}

static int 
jfs_link(const char *from, const char *to)
{
  char *jfs_path_from;
  char *jfs_path_to;

  int rc;
  
  jfs_path_from = jfs_realpath(from);
  jfs_path_to = jfs_realpath(to);
  rc = jfs_file_link(jfs_path_from, jfs_path_to);
  
  if(jfs_path_from) {
    free(jfs_path_from);
  }
  if(jfs_path_to) {
    free(jfs_path_to);
  }

  if(rc) {
    log_error("jfs_link---from:%s, to:%s, error:%d\n", from, to, rc);
    return rc;
  }

  return 0;
}

static int 
jfs_chmod(const char *path, mode_t mode)
{
  char *jfs_path;
  int rc;
  
  jfs_path = jfs_realpath(path);
  rc = jfs_security_chmod(jfs_path, mode);
  free(jfs_path);

  if(rc) {
	log_error("jfs_chmod---path:%s, mode:%d, error:%d\n", path, mode, rc);
    return rc;
  }

  return 0;
}

static int 
jfs_chown(const char *path, uid_t uid, gid_t gid)
{
  int rc;
  
  rc = jfs_security_chown(path, uid, gid);
  if(rc) {
	log_error("jfs_chown---path:%s, uid:%d, gid:%d, error:%d\n", path, uid, gid, rc);
    return rc;
  }

  return 0;
}

static int 
jfs_truncate(const char *path, off_t size)
{
  char *jfs_path;
  int rc;
  
  jfs_path = jfs_realpath(path);
  rc = jfs_file_truncate(jfs_path, size);
  free(jfs_path);

  if(rc) {
	log_error("jfs_truncate---path:%s, size:%d, error:%d\n", path, size, rc);
    return rc;
  }

  return 0;
}

static int 
jfs_utimens(const char *path, const struct timespec ts[2])
{
  struct timeval tv[2];
  char *jfs_path;
  int rc;
  
  tv[0].tv_sec = ts[0].tv_sec;
  tv[0].tv_usec = ts[0].tv_nsec / 1000;
  tv[1].tv_sec = ts[1].tv_sec;
  tv[1].tv_usec = ts[1].tv_nsec / 1000;

  jfs_path = jfs_realpath(path);
  rc = utimes(jfs_path, tv);
  free(jfs_path);

  if(rc) {
	log_error("jfs_utimens---path:%s, error:%d\n", path, rc);
    return rc;
  }

  return 0;
}

static int 
jfs_open(const char *path, struct fuse_file_info *fi)
{
  char *jfs_path;
  int fd;
  
  jfs_path = jfs_realpath(path);
  fd = jfs_file_open(jfs_path, fi->flags, 0);
  free(jfs_path);

  if(fd < 0) {
	log_error("jfs_open---path:%s, error:%d\n", path, fd);
    return fd;
  }

  fi->fh = fd;
  fi->direct_io = 1;

  return 0;
}

static int 
jfs_read(const char *path, char *buf, size_t size, off_t offset,
		 struct fuse_file_info *fi)
{
  int rc;

  (void) path;
  
  rc = pread(fi->fh, buf, size, offset);
  if(rc == -1) {
	log_error("jfs_read---error:%d\n", -errno);
    return -errno;
  }

  return rc;
}

static int 
jfs_write(const char *path, const char *buf, size_t size,
		  off_t offset, struct fuse_file_info *fi)
{
  int rc;
  
  (void) path;
  
  rc = pwrite(fi->fh, buf, size, offset);
  if (rc == -1) {
	log_error("jfs_write---error:%d\n", -errno);
    return -errno;
  }

  return rc;
}

static int 
jfs_statfs(const char *path, struct statvfs *stbuf)
{
  char *jfs_path;
  int rc;
  
  jfs_path = jfs_realpath(path);
  rc = jfs_file_statfs(jfs_path, stbuf);
  free(jfs_path);

  if(rc < 0) {
	log_error("jfs_statfs---path:%s, error:%d\n", path, rc);
    return rc;
  }

  return 0;
}

static int 
jfs_fsync(const char *path, int isdatasync,
		  struct fuse_file_info *fi)
{
  int rc;

  (void) path;
#ifndef HAVE_FDATASYNC
  (void) isdatasync;
  
#else
  if(isdatasync)
	rc = fdatasync(fi->fh);
  else
#endif
	rc = fsync(fi->fh);

  if(rc < 0) {
    log_error("jfs_fsync---path:%s, error:%d\n", path, -errno);
	return -errno;
  }

  return 0;
}

static int 
jfs_setxattr(const char *path, const char *name, const char *value,
			 size_t size, int flags)
{
  char *jfs_path;
  int rc;

  jfs_path = jfs_realpath(path);
  rc = jfs_meta_setxattr(jfs_path, name, value, size, flags);
  free(jfs_path);

  if(rc) {
    log_error("jfs_setxattr---path:%s, name:%s, value:%s, flags:%d, error:%d\n", 
              path, name, value, flags, rc);
    return rc;
  }

  return 0;
}

static int
jfs_getxattr(const char *path, const char *name, char *value,
			 size_t size)
{
  char *jfs_path;
  int rc;

  jfs_path = jfs_realpath(path);
  rc = jfs_meta_getxattr(jfs_path, name, value, size);
  free(jfs_path);

  if(rc < 0) {
    log_error("jfs_getxattr---path:%s, name:%s, error:%d\n", path, name, rc);
  }
  
  return rc;
}

static int 
jfs_listxattr(const char *path, char *list, size_t size)
{
  char *jfs_path;
  int rc;
  
  jfs_path = jfs_realpath(path);
  rc = jfs_meta_listxattr(jfs_path, list, size);
  free(jfs_path);

  if(rc < 0) {
    log_error("jfs_listxattr---path:%s, error:%d\n", path, rc);
  }

  return rc;
}

static int 
jfs_removexattr(const char *path, const char *name)
{
  char *jfs_path;
  int rc;
  
  jfs_path = jfs_realpath(path);
  rc = jfs_meta_removexattr(jfs_path, name);
  free(jfs_path);

  if(rc) {
    log_error("jfs_removexattr---path:%s, name:%s, error:%d\n", path, name, rc);
    return rc;
  }

  return 0;
}

static int
jfs_release(const char *path, struct fuse_file_info *fi)
{
  (void) path;

  close(fi->fh);

  return 0;
}

static int
jfs_lock(const char *path, struct fuse_file_info *fi,
         int cmd, struct flock *lock)
{
  (void) path;

  return ulockmgr_op(fi->fh, cmd, lock, &fi->lock_owner,
                     sizeof(fi->lock_owner));
}

static int
jfs_fgetattr(const char *path, struct stat *stbuf,
             struct fuse_file_info *fi)
{
	int rc;

	(void) path;

	rc = fstat(fi->fh, stbuf);
	if(rc) {
      return -errno;
    }
    
	return 0;
}

static int 
jfs_ftruncate(const char *path, off_t size,
              struct fuse_file_info *fi)
{
	int rc;

	(void) path;

	rc = ftruncate(fi->fh, size);
	if(rc) {
      return -errno;
    }

	return 0;
}

static int 
jfs_flush(const char *path, struct fuse_file_info *fi)
{
	int rc;

	(void) path;
	
	rc = close(dup(fi->fh));
	if(rc) {
      return -errno;
    }

	return 0;
}

static struct fuse_operations jfs_oper = {
  .getattr	    = jfs_getattr,
  .fgetattr     = jfs_fgetattr,
  .access	    = jfs_access,
  .readlink	    = jfs_readlink,
  .opendir      = jfs_opendir,
  .releasedir   = jfs_releasedir,
  .readdir	    = jfs_readdir,
  .create       = jfs_create,
  .mknod	    = jfs_mknod,
  .mkdir	    = jfs_mkdir,
  .symlink	    = jfs_symlink,
  .unlink       = jfs_unlink,
  .rmdir        = jfs_rmdir,
  .rename       = jfs_rename,
  .link		    = jfs_link,
  .chmod        = jfs_chmod,
  .chown        = jfs_chown,
  .truncate	    = jfs_truncate,
  .ftruncate    = jfs_ftruncate,
  .utimens	    = jfs_utimens,
  .open		    = jfs_open,
  .read		    = jfs_read,
  .write        = jfs_write,
  .statfs       = jfs_statfs,
  .fsync        = jfs_fsync,
  .init         = jfs_init,
  .destroy      = jfs_destroy,
  .setxattr	    = jfs_setxattr,
  .getxattr	    = jfs_getxattr,
  .listxattr	= jfs_listxattr,
  .removexattr	= jfs_removexattr,
  .release      = jfs_release,
  .lock         = jfs_lock,
  .flush        = jfs_flush,

  .flag_nullpath_ok = 1,
};

int 
main(int argc, char *argv[])
{
  size_t length;

  int i;
  int rc;
  
  for(i = 1; (i < argc) && (argv[i][0] == '-'); i++);

  if((argc - i) < 4) {
	printf("format: joinfs querypath mountpath logpath dbpath\n");
    exit(EXIT_FAILURE);
  }

  joinfs_context.querypath = realpath(argv[i], NULL);
  joinfs_context.querypath_len = strlen(joinfs_context.querypath);
  
  joinfs_context.mountpath = realpath(argv[i + 1], NULL);
  joinfs_context.mountpath_len = strlen(joinfs_context.mountpath);

  length = strlen(argv[i + 2]) + 1;
  joinfs_context.logpath = malloc(sizeof(*joinfs_context.logpath) * length);
  if(!joinfs_context.logpath) {
    printf("joinFS failed to start because the system is out of memory.\n");
    exit(EXIT_FAILURE);
  }
  strncpy(joinfs_context.logpath, argv[i + 2], length);

  length = strlen(argv[i + 3]) + 1;
  joinfs_context.dbpath = malloc(sizeof(*joinfs_context.dbpath) * length);
  if(!joinfs_context.dbpath) {
    printf("joinFS failed to start because the system is out of memory.\n");
    exit(EXIT_FAILURE);
  }
  strncpy(joinfs_context.dbpath, argv[i + 3], length);
  
  argc = 2;
  argv[1] = joinfs_context.mountpath;
  
  printf("Starting joinFS, mounted at: %s\n", argv[1]);
  rc = fuse_main(argc, argv, &jfs_oper, NULL);;

  if(rc) {
    printf("joinFS start up failed. FUSE error code=%d.\n", rc);
  }

  return rc;
}
