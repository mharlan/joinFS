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

#define JFS_THREAD_MIN    10
#define JFS_THREAD_MAX    200
#define JFS_THREAD_LINGER 100
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
#include "jfs_file_cache.h"
#include "jfs_path_cache.h"
#include "jfs_datapath_cache.h"
#include "jfs_key_cache.h"
#include "jfs_meta_cache.h"
#include "thr_pool.h"
#include "sqlitedb.h"
#include "joinfs.h"

#include <sqlite3.h>
#include <fuse.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/xattr.h>

static thr_pool_t *jfs_read_pool;
static thr_pool_t *jfs_write_pool;

/*
 * Get a joinFS real path.
 *
 * The realpath must be freed.
 */
static char *
jfs_realpath(const char *path)
{
  char *jfs_path;
  char *jfs_rpath;

  int path_len;

  log_error("Called jfs_realpath, path:%s\n", path);

  path_len = strlen(path) + JFS_CONTEXT->querypath_len + 2;
  jfs_path = malloc(sizeof(*jfs_path) * path_len);
  if(!jfs_path) {
	log_error("Failed to allocate memory for jfs_realpath.\n");
	return 0;
  }

  if(path[0] == '/') {
	snprintf(jfs_path, path_len, "%s%s", JFS_CONTEXT->querypath, path);
  }
  else if(path == NULL) {
	snprintf(jfs_path, path_len, "%s/", JFS_CONTEXT->querypath);
  }
  else {
	snprintf(jfs_path, path_len, "%s/%s", JFS_CONTEXT->querypath, path);
  }
  
  jfs_rpath = realpath(jfs_path, NULL);
  if(!jfs_rpath) {
	log_error("realpath failed, resolved path:%s to jfs_path:%s\n", path, jfs_path);
	return jfs_path;
  }

  log_error("Transformed path:%s to jfs_path:%s to jfs_realpath:%s\n", path, jfs_path, jfs_rpath);
  free(jfs_path);

  return jfs_rpath;
}

/*
 * Perform a read database operation.
 */
int 
jfs_read_pool_queue(struct jfs_db_op *db_op)
{
  return jfs_pool_queue(jfs_read_pool, db_op);
}

/*
 * Perform a write database operation.
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
  int rc;

  log_init();
  log_error("Starting joinFS. FUSE Major=%d Minor=%d\n",
			conn->proto_major, conn->proto_minor);

  /* initialize caches */
  jfs_file_cache_init();
  jfs_path_cache_init();
  jfs_datapath_cache_init();
  jfs_key_cache_init();
  jfs_meta_cache_init();
  
  /* start sqlite in multithreaded mode */
  rc = sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
  if(rc != SQLITE_OK) {
	log_error("Failed to configuring multithreading for SQLITE.\n");
	exit(EXIT_FAILURE);
  }

  rc = sqlite3_initialize();
  if(rc != SQLITE_OK) {
	log_error("Failed to initialize SQLITE.\n");
	exit(EXIT_FAILURE);
  }

  log_error("SQLITE started.\n");

  jfs_read_pool = jfs_pool_create(JFS_THREAD_MIN, JFS_THREAD_MAX, 
								  JFS_THREAD_LINGER, NULL, 
								  SQLITE_OPEN_READONLY);
  if(!jfs_read_pool) {
	log_error("Failed to allocate READ pool.\n");
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
	exit(EXIT_FAILURE);
  }
  
  log_error("Thread pools started.\n");
  
  return JFS_CONTEXT;
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

  jfs_file_cache_destroy();
  printf("File cache destroyed.\n");
  jfs_path_cache_destroy();
  printf("Path cache destroyed.\n");
  jfs_datapath_cache_destroy();
  printf("Datapath cache destroyed.\n");
  jfs_key_cache_destroy();
  printf("Key cache destroyed.\n");
  jfs_meta_cache_destroy();
  printf("Meta cache destroyed.\n");

  log_error("joinFS shutdown. Passed arg:%d.\n", arg);
  log_destroy();

  free(JFS_CONTEXT);
}

static int 
jfs_getattr(const char *path, struct stat *stbuf)
{
  char *jfs_path;
  int rc;

  log_error("Called jfs_getattr, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  rc = jfs_file_getattr(jfs_path, stbuf);
  free(jfs_path);

  if(rc) {
	log_error("Error occured, errno:%d\n", rc);
    return rc;
  }
  
  return 0;
}

static int 
jfs_access(const char *path, int mask)
{
  char *jfs_path;
  int rc;
  
  log_error("Called jfs_access, path:%s, mask:%d\n", path, mask);

  jfs_path = jfs_realpath(path);
  rc = jfs_security_access(jfs_path, mask);
  free(jfs_path);

  if(rc) {
	log_error("Error occured, errno:%d\n", rc);
    return rc;
  }
  
  return 0;
}

static int 
jfs_readlink(const char *path, char *buf, size_t size)
{
  char *jfs_path;
  int rc;

  log_error("Called jfs_readlink, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  rc = readlink(jfs_path, buf, size - 1);
  free(jfs_path);

  if(rc) {
	log_error("Error occured, errno:%d\n", rc);
    return rc;
  }

  buf[rc] = '\0';
  return 0;
}


static int 
jfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			off_t offset, struct fuse_file_info *fi)
{
  char *jfs_path;
  int rc;

  (void) offset;

  log_error("Called jfs_readdir, path:%s\n", path);
  
  jfs_path = jfs_realpath(path);
  rc = jfs_dir_readdir(jfs_path, buf, filler);
  free(jfs_path);

  if(rc) {
	return rc;
  }

  return 0;
}

static int
jfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
  char *jfs_path;
  int fd;

  log_error("Called jfs_create, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  fd = jfs_file_create(jfs_path, mode);
  free(jfs_path);

  if(fd < 0) {
	log_error("Error occured, errno:%d\n", fd);
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

  log_error("Called jfs_mknod, path:%s mode:%d\n", path, mode);

  jfs_path = jfs_realpath(path);
  /* On Linux this could just be 'mknod(path, mode, rdev)' but this
     is more portable */
  if(S_ISREG(mode)) {
	rc = jfs_file_mknod(jfs_path, mode);
  } 
  else if(S_ISFIFO(mode)) {
    rc = mkfifo(jfs_path, mode);
  }
  else {
    rc = mknod(jfs_path, mode, rdev);
  }
  free(jfs_path);

  if(rc == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }
  else if(rc < -1) {
	return rc;
  }

  return 0;
}

static int 
jfs_mkdir(const char *path, mode_t mode)
{
  char *jfs_path;
  int rc;
  
  log_error("Called jfs_mkdir, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  rc = jfs_dir_mkdir(jfs_path, mode);
  free(jfs_path);

  if(rc) {
	log_error("Error occured, errno:%d\n", rc);
    return rc;
  }

  return 0;
}

static int
jfs_unlink(const char *path)
{
  char *jfs_path;
  int rc;
  
  log_error("Called jfs_unlink, path:%s\n", path);
  
  jfs_path = jfs_realpath(path);
  rc = jfs_file_unlink(jfs_path);
  free(jfs_path);

  if(rc) {
	log_error("Error occured, errno:%d\n", rc);
    return rc;
  }

  return 0;
}

static int
jfs_rmdir(const char *path)
{
  char *jfs_path;
  int rc;

  log_error("Called jfs_rmdir, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  rc = jfs_dir_rmdir(jfs_path);
  free(jfs_path);

  if(rc) {
	log_error("Error occured, errno:%d\n", rc);
    return rc;
  }

  return 0;
}

static int 
jfs_symlink(const char *from, const char *to)
{
  char *jfs_path_from;
  char *jfs_path_to;
  int rc;

  log_error("Called jfs_symlink, from:%s to:%s\n", from, to);
  
  jfs_path_from = jfs_realpath(from);
  jfs_path_to = jfs_realpath(to);
  rc = symlink(jfs_path_from, jfs_path_to);
  free(jfs_path_from);
  free(jfs_path_to);

  if(rc) {
	log_error("Error occured, errno:%d\n", rc);
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
  free(jfs_path_from);
  free(jfs_path_to);

  if(rc) {
	log_error("Error occured, errno:%d\n", rc);
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

  log_error("Called jfs_link, from:%s to:%s\n", from, to);

  jfs_path_from = jfs_realpath(from);
  jfs_path_to = jfs_realpath(to);
  rc = link(jfs_path_from, jfs_path_to);
  free(jfs_path_from);
  free(jfs_path_to);

  if(rc) {
	log_error("Error occured, errno:%d\n", rc);
    return rc;
  }

  return 0;
}

static int 
jfs_chmod(const char *path, mode_t mode)
{
  char *jfs_path;
  int rc;

  log_error("Called jfs_chmod, path:%s mode:%d\n", path, mode);

  jfs_path = jfs_realpath(path);
  rc = jfs_security_chmod(jfs_path, mode);
  free(jfs_path);

  if(rc) {
	log_error("Error occured, errno:%d\n", rc);
    return rc;
  }

  return 0;
}

static int 
jfs_chown(const char *path, uid_t uid, gid_t gid)
{
  int rc;

  log_error("Called jfs_chmod, path:%s uid:%d gid:%d\n", path, uid, gid);
  rc = jfs_security_chown(path, uid, gid);
  if(rc) {
	log_error("Error occured, errno:%d\n", rc);
    return rc;
  }

  return 0;
}

static int 
jfs_truncate(const char *path, off_t size)
{
  char *jfs_path;
  int rc;

  log_error("Called jfs_truncate, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  rc = jfs_file_truncate(jfs_path, size);
  free(jfs_path);

  if(rc) {
	log_error("Error occured, errno:%d\n", rc);
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

  log_error("Called jfs_utimens, path:%s\n", path);
  tv[0].tv_sec = ts[0].tv_sec;
  tv[0].tv_usec = ts[0].tv_nsec / 1000;
  tv[1].tv_sec = ts[1].tv_sec;
  tv[1].tv_usec = ts[1].tv_nsec / 1000;

  jfs_path = jfs_realpath(path);
  rc = utimes(jfs_path, tv);
  free(jfs_path);

  if(rc) {
	log_error("Error occured, errno:%d\n", rc);
    return rc;
  }

  return 0;
}

static int 
jfs_open(const char *path, struct fuse_file_info *fi)
{
  char *jfs_path;
  int fd;

  log_error("Called jfs_file_open, path:%s, flags:%d\n", path, fi->flags);

  jfs_path = jfs_realpath(path);
  fd = jfs_file_open(jfs_path, fi->flags);
  free(jfs_path);

  if(fd < 0) {
	log_error("Error occured, errno:%d\n", fd);
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

  log_error("Called jfs_read, path:%s, fi->fh:%llu\n", path, fi->fh);

  rc = pread(fi->fh, buf, size, offset);

  if (rc == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  log_error("Pread return %d bytes in buf:%s, expected:%d\n", rc, buf, size);

  return rc;
}

static int 
jfs_write(const char *path, const char *buf, size_t size,
		  off_t offset, struct fuse_file_info *fi)
{
  int rc;

  log_error("Called jfs_write, path:%s fi->fh:%llu\n", path, fi->fh);

  rc = pwrite(fi->fh, buf, size, offset);
  if (rc == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return rc;
}

static int 
jfs_statfs(const char *path, struct statvfs *stbuf)
{
  char *jfs_path;
  int rc;

  log_error("Called jfs_statvfs, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  rc = statvfs(jfs_path, stbuf);
  free(jfs_path);

  if(rc == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return 0;
}

static int 
jfs_fsync(const char *path, int isdatasync,
		  struct fuse_file_info *fi)
{
  char *jfs_path;
  int rc;

  jfs_path = jfs_realpath(path);

  if(isdatasync) {
	rc = fdatasync(fi->fh);
  }
  else {
	rc = fsync(fi->fh);
  }

  if(rc < 0) {
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

  log_error("Called jfs_setxattr, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  rc = jfs_meta_setxattr(jfs_path, name, value, size, flags);
  free(jfs_path);

  if(rc) {
	log_error("Error occured, errno:%d\n", rc);
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

  log_error("Called jfs_getxattr, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  rc = jfs_meta_getxattr(jfs_path, name, value, size);
  free(jfs_path);

  if(rc < 0) {
	log_error("Error occured, errno:%d\n", rc);
  }

  return rc;
}

static int 
jfs_listxattr(const char *path, char *list, size_t size)
{
  char *jfs_path;
  int rc;

  log_error("Called jfs_listxattr, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  rc = jfs_meta_listxattr(jfs_path, list, size);
  free(jfs_path);

  if(rc < 0) {
	log_error("Error occured, errno:%d\n", rc);
  }

  return rc;
}

static int 
jfs_removexattr(const char *path, const char *name)
{
  char *jfs_path;
  int rc;

  log_error("Called jfs_removexattr, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  rc = jfs_meta_removexattr(jfs_path, name);
  free(jfs_path);

  if(rc) {
	log_error("Error occured, errno:%d\n", rc);
    return rc;
  }

  return 0;
}

static struct fuse_operations jfs_oper = {
  .getattr	    = jfs_getattr,
  .access	    = jfs_access,
  .readlink	    = jfs_readlink,
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
  .removexattr	= jfs_removexattr
};

int 
main(int argc, char *argv[])
{
  int i;
  int rc;
  struct jfs_context *jfs_context;

  jfs_context = malloc(sizeof(*jfs_context));
  if(!jfs_context) {
	printf("Failed to allocate space for jfs_context.\n");
	exit(EXIT_FAILURE);
  }

  printf("Passed %d args\n", argc);

  for(i = 1; (i < argc) && (argv[i][0] == '-'); i++);

  if((argc - i) < 3) {
	printf("format: joinfs querydir datadir mountdir\n");
  }

  jfs_context->querypath = realpath(argv[1], NULL);
  jfs_context->querypath_len = strlen(jfs_context->querypath);

  jfs_context->datapath = realpath(argv[2], NULL);
  jfs_context->datapath_len = strlen(jfs_context->datapath);
  
  jfs_context->mountpath = realpath(argv[3], NULL);
  jfs_context->mountpath_len = strlen(jfs_context->mountpath);
  
  printf("querydir:%s\n", jfs_context->querypath);
  printf("datadir:%s\n", jfs_context->datapath);
  printf("mountdir:%s\n", jfs_context->mountpath);

  argc = 4;
  argv[1] = "-d";
  argv[2] = "-s";
  argv[3] = jfs_context->mountpath;
 
  /*
  argc = 2;
  argv[1] = jfs_context->mountpath;
  */

  printf("Starting joinFS, mountpath:%s\n", argv[1]);
  rc = fuse_main(argc, argv, &jfs_oper, jfs_context);;
  printf("joinFS stopped. Return code=%d.\n", rc);

  return rc;
}
