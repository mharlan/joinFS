/*
 * joinFS: Relational filesystem for FUSE
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 *
 * 30% Demo
 *
 * Note: Static file support.
 */

#define JFS_THREAD_MIN    10
#define JFS_THREAD_MAX    200
#define JFS_THREAD_LINGER 100
#define FUSE_USE_VERSION  26
#define JFS_MOUNT_PATH    "/home/joinfs/git/joinFS/demo/"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include "error_log.h"
#include "jfs_file.h"
#include "thr_pool.h"
#include "sqlitedb.h"
#include "joinfs.h"

#include <sqlite3.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

static thr_pool_t *jfs_read_pool;
static thr_pool_t *jfs_write_pool;

int 
jfs_read_pool_queue(struct jfs_db_op *db_op)
{
  return jfs_pool_queue(jfs_read_pool, db_op);
}

int 
jfs_write_pool_queue(struct jfs_db_op *db_op)
{
  return jfs_pool_queue(jfs_write_pool, db_op);
}

static void * 
jfs_init(struct fuse_conn_info *conn)
{
  int rc;

  log_error("Starting joinFS. FUSE Major=%d Minor=%d\n",
			conn->proto_major, conn->proto_minor);
  
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

  jfs_write_pool = jfs_pool_create(1, 1, JFS_THREAD_LINGER, 
								   NULL, SQLITE_OPEN_READWRITE);
  if(!jfs_write_pool) {
	log_error("Failed to allocate WRITE pool.\n");
	exit(EXIT_FAILURE);
  }
  
  log_error("Thread pools started.\n");
  
  return 0;
}

static void 
jfs_destroy(void *arg)
{
  int rc;

  /* stop all reads */
  jfs_pool_destroy(jfs_read_pool);
  
  /* let writes propogate */
  jfs_pool_wait(jfs_read_pool);
  jfs_pool_destroy(jfs_write_pool);

  rc = sqlite3_shutdown();
  if(rc != SQLITE_OK) {
	log_error("SQLITE shutdown FAILED!!!\n");
  }

  log_error("joinFS shutdown. Passed arg:%d.\n", arg);
}

static int 
jfs_getattr(const char *path, struct stat *stbuf)
{
  int res;
  
  res = lstat(path, stbuf);
  if (res == -1) {
    return -errno;
  }
  
  return 0;
}

static int 
jfs_access(const char *path, int mask)
{
  int res;
  
  res = access(path, mask);
  if (res == -1) {
    return -errno;
  }
  
  return 0;
}

static int 
jfs_readlink(const char *path, char *buf, size_t size)
{
  int res;

  res = readlink(path, buf, size - 1);
  if (res == -1) {
    return -errno;
  }

  buf[res] = '\0';
  return 0;
}


static int 
jfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			off_t offset, struct fuse_file_info *fi)
{
  DIR *dp;
  struct dirent *de;

  (void) offset;
  (void) fi;

  dp = opendir(path);
  if (dp == NULL) {
    return -errno;
  }

  while ((de = readdir(dp)) != NULL) {
    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_ino = de->d_ino;
    st.st_mode = de->d_type << 12;

    if (filler(buf, de->d_name, &st, 0)) {
      break;
	}
  }

  closedir(dp);
  return 0;
}

static int
jfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
  int res;

  /* On Linux this could just be 'mknod(path, mode, rdev)' but this
     is more portable */
  if (S_ISREG(mode)) {
    res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);

    if (res >= 0) {
	  res = jfs_s_file_create(path, res, mode);
      res = close(res);
	}
  } 
  else if (S_ISFIFO(mode)) {
    res = mkfifo(path, mode);
  }
  else {
    res = mknod(path, mode, rdev);
  }
  if (res == -1) {
    return -errno;
  }

  return 0;
}

static int 
jfs_mkdir(const char *path, mode_t mode)
{
  int res;

  res = mkdir(path, mode);
  if (res == -1) {
    return -errno;
  }

  return 0;
}

static int
jfs_unlink(const char *path)
{
  int res;

  res = unlink(path);
  if (res == -1) {
    return -errno;
  }

  return 0;
}

static int
jfs_rmdir(const char *path)
{
  int res;

  res = rmdir(path);
  if (res == -1) {
    return -errno;
  }

  return 0;
}

static int 
jfs_symlink(const char *from, const char *to)
{
  int res;

  res = symlink(from, to);
  if (res == -1) {
    return -errno;
  }

  return 0;
}

static int 
jfs_rename(const char *from, const char *to)
{
  int res;

  res = rename(from, to);
  if (res == -1) {
    return -errno;
  }

  return 0;
}

static int 
jfs_link(const char *from, const char *to)
{
  int res;

  res = link(from, to);
  if (res == -1) {
    return -errno;
  }

  return 0;
}

static int 
jfs_chmod(const char *path, mode_t mode)
{
  int res;

  res = chmod(path, mode);
  if (res == -1) {
    return -errno;
  }

  return 0;
}

static int 
jfs_chown(const char *path, uid_t uid, gid_t gid)
{
  int res;

  res = lchown(path, uid, gid);
  if (res == -1) {
    return -errno;
  }

  return 0;
}

static int 
jfs_truncate(const char *path, off_t size)
{
  int res;

  res = truncate(path, size);
  if (res == -1) {
    return -errno;
  }

  return 0;
}

static int 
jfs_utimens(const char *path, const struct timespec ts[2])
{
  int res;
  struct timeval tv[2];

  tv[0].tv_sec = ts[0].tv_sec;
  tv[0].tv_usec = ts[0].tv_nsec / 1000;
  tv[1].tv_sec = ts[1].tv_sec;
  tv[1].tv_usec = ts[1].tv_nsec / 1000;

  res = utimes(path, tv);
  if (res == -1) {
    return -errno;
  }

  return 0;
}

static int 
jfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
  return 0;
}

static int 
jfs_open(const char *path, struct fuse_file_info *fi)
{
  int res;

  res = open(path, fi->flags);
  if (res == -1) {
    return -errno;
  }

  close(res);
  return 0;
}

static int 
jfs_read(const char *path, char *buf, size_t size, off_t offset,
		 struct fuse_file_info *fi)
{
  int fd;
  int res;

  (void) fi;
  fd = open(path, O_RDONLY);
  if (fd == -1) {
    return -errno;
  }

  res = pread(fd, buf, size, offset);
  if (res == -1) {
    res = -errno;
  }

  close(fd);
  return res;
}

static int 
jfs_write(const char *path, const char *buf, size_t size,
		  off_t offset, struct fuse_file_info *fi)
{
  int fd;
  int res;

  (void) fi;
  fd = open(path, O_WRONLY);
  if (fd == -1) {
    return -errno;
  }

  res = pwrite(fd, buf, size, offset);
  if (res == -1) {
    res = -errno;
  }

  close(fd);
  return res;
}

static int 
jfs_statfs(const char *path, struct statvfs *stbuf)
{
  int res;

  res = statvfs(path, stbuf);
  if (res == -1) {
    return -errno;
  }

  return 0;
}

static int 
jfs_release(const char *path, struct fuse_file_info *fi)
{
  /* Just a stub.	 This method is optional and can safely be left
     unimplemented */
  (void) path;
  (void) fi;

  return 0;
}

static int 
jfs_fsync(const char *path, int isdatasync,
		  struct fuse_file_info *fi)
{
  /* Just a stub.	 This method is optional and can safely be left
     unimplemented */
  (void) path;
  (void) isdatasync;
  (void) fi;

  return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int 
jfs_setxattr(const char *path, const char *name, const char *value,
			 size_t size, int flags)
{
  int res;

  res = lsetxattr(path, name, value, size, flags);
  if (res == -1) {
    return -errno;
  }

  return 0;
}

static int
jfs_getxattr(const char *path, const char *name, char *value,
			 size_t size)
{
  int res;

  res = lgetxattr(path, name, value, size);
  if (res == -1) {
    return -errno;
  }

  return res;
}

static int 
jfs_listxattr(const char *path, char *list, size_t size)
{
  int res;

  res = llistxattr(path, list, size);
  if (res == -1) {
    return -errno;
  }

  return res;
}

static int 
jfs_removexattr(const char *path, const char *name)
{
  int res;

  res = lremovexattr(path, name);
  if (res == -1) {
    return -errno;
  }

  return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations jfs_oper = {
  .getattr	    = jfs_getattr,
  .access	    = jfs_access,
  .readlink	    = jfs_readlink,
  .readdir	    = jfs_readdir,
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
  .create       = jfs_create,
  .open		    = jfs_open,
  .read		    = jfs_read,
  .write        = jfs_write,
  .statfs       = jfs_statfs,
  .release	    = jfs_release,
  .fsync        = jfs_fsync,
  .init         = jfs_init,
  .destroy      = jfs_destroy,
#ifdef HAVE_SETXATTR
  .setxattr	    = jfs_setxattr,
  .getxattr	    = jfs_getxattr,
  .listxattr	= jfs_listxattr,
  .removexattr	= jfs_removexattr,
#endif
};

int 
main(int argc, char *argv[])
{
  int rc;
  struct jfs_context *jfs_context;

  jfs_context = calloc(sizeof(jfs_context), 1);
  jfs_context->rootdir = realpath(JFS_MOUNT_PATH, NULL);
  jfs_context->rootlen = strlen(jfs_context->rootdir);
  
  log_error("Starting FUSE.\n");
  rc = fuse_main(argc, argv, &jfs_oper, jfs_context);
  log_error("Fuse returned, status=%s.\n", rc);

  return rc;
}
