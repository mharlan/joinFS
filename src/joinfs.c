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
#define FUSE_USE_VERSION  27

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include "error_log.h"
#include "jfs_file.h"
#include "jfs_file_cache.h"
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

/*
 * Get a joinFS real path.
 *
 * The realpath must be freed.
 */
static char *
jfs_realpath(const char *path)
{
  char *jfs_realpath;
  int path_len;

  log_error("Called jfs_realpath, path:%s\n", path);

  path_len = strlen(path) + JFS_CONTEXT->querypath_len + 2;
  jfs_realpath = malloc(sizeof(*jfs_realpath) * path_len);
  if(!jfs_realpath) {
	log_error("Failed to allocate memory for jfs_realpath.\n");
  }
  else {
	if(path[0] == '/') {
	  snprintf(jfs_realpath, path_len, "%s%s", JFS_CONTEXT->querypath, path);
	}
	else if(path == NULL) {
	  snprintf(jfs_realpath, path_len, "%s/", JFS_CONTEXT->querypath);
	}
	else {
	  snprintf(jfs_realpath, path_len, "%s/%s", JFS_CONTEXT->querypath, path);
	}

	log_error("Computed realpath:%s\n", jfs_realpath);
  }

  return jfs_realpath;
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
  int rc;

  log_init();
  log_error("Starting joinFS. FUSE Major=%d Minor=%d\n",
			conn->proto_major, conn->proto_minor);

  /* initialize the static file cache */
  jfs_file_cache_init();
  
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
  
  return JFS_CONTEXT;
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

  jfs_file_cache_destroy();

  log_error("joinFS shutdown. Passed arg:%d.\n", arg);
  log_destroy();

  free(JFS_CONTEXT);
}

static int 
jfs_getattr(const char *path, struct stat *stbuf)
{
  int res;
  char *jfs_path;

  log_error("Called jfs_getattr, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  res = lstat(jfs_path, stbuf);
  free(jfs_path);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }
  
  return 0;
}

static int 
jfs_access(const char *path, int mask)
{
  char *jfs_path;
  int res;
  
  log_error("Called jfs_access, path:%s, mask:%d\n", path, mask);

  jfs_path = jfs_realpath(path);
  res = access(jfs_path, mask);
  free(jfs_path);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }
  
  return 0;
}

static int 
jfs_readlink(const char *path, char *buf, size_t size)
{
  char *jfs_path;
  int res;

  log_error("Called jfs_readlink, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  res = readlink(jfs_path, buf, size - 1);
  free(jfs_path);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
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
  char *jfs_path;

  (void) offset;
  (void) fi;

  log_error("Called jfs_readdir, path:%s\n", path);
  
  jfs_path = jfs_realpath(path);
  dp = opendir(jfs_path);
  free(jfs_path);

  log_error("DIR:%d, for jfs_path:%s\n", dp, jfs_path);

  if (dp == NULL) {
	log_error("Error occured, errno:%d\n", -errno);
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
jfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
  char *jfs_path;
  int fd;

  log_error("Called jfs_create, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  fd = jfs_file_create(jfs_path, mode);
  free(jfs_path);
  fi->fh = fd;

  if(fd < 0) {
	log_error("Error occured, errno:%d\n", -errno);
	return -errno;
  }

  return 0;
}

static int
jfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
  char *jfs_path;
  int res;

  log_error("Called jfs_mknod, path:%s mode:%d\n", path, mode);

  jfs_path = jfs_realpath(path);
  /* On Linux this could just be 'mknod(path, mode, rdev)' but this
     is more portable */
  if(S_ISREG(mode)) {
	res = jfs_file_mknod(jfs_path, mode);
	//res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
  } 
  else if(S_ISFIFO(mode)) {
    res = mkfifo(jfs_path, mode);
  }
  else {
    res = mknod(jfs_path, mode, rdev);
  }

  free(jfs_path);

  if(res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return 0;
}

static int 
jfs_mkdir(const char *path, mode_t mode)
{
  char *jfs_path;
  int res;
  
  log_error("Called jfs_mkdir, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  res = mkdir(jfs_path, mode);
  free(jfs_path);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return 0;
}

static int
jfs_unlink(const char *path)
{
  char *jfs_path;
  int res;
  
  log_error("Called jfs_unlink, path:%s\n", path);
  
  jfs_path = jfs_realpath(path);
  res = jfs_file_unlink(jfs_path);
  free(jfs_path);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return 0;
}

static int
jfs_rmdir(const char *path)
{
  char *jfs_path;
  int res;

  log_error("Called jfs_rmdir, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  res = rmdir(jfs_path);
  free(jfs_path);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return 0;
}

static int 
jfs_symlink(const char *from, const char *to)
{
  char *jfs_path_from;
  char *jfs_path_to;
  int res;

  log_error("Called jfs_symlink, from:%s to:%s\n", from, to);
  
  jfs_path_from = jfs_realpath(from);
  jfs_path_to = jfs_realpath(to);
  res = symlink(jfs_path_from, jfs_path_to);
  free(jfs_path_from);
  free(jfs_path_to);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return 0;
}

static int 
jfs_rename(const char *from, const char *to)
{
  char *jfs_path_from;
  char *jfs_path_to;
  int res;
  
  log_error("Called jfs_rename, from:%s to:%s\n", from, to);

  jfs_path_from = jfs_realpath(from);
  jfs_path_to = jfs_realpath(to);
  res = jfs_file_rename(jfs_path_from, jfs_path_to);
  free(jfs_path_from);
  free(jfs_path_to);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return 0;
}

static int 
jfs_link(const char *from, const char *to)
{
  char *jfs_path_from;
  char *jfs_path_to;
  int res;

  log_error("Called jfs_link, from:%s to:%s\n", from, to);

  jfs_path_from = jfs_realpath(from);
  jfs_path_to = jfs_realpath(to);
  res = link(jfs_path_from, jfs_path_to);
  free(jfs_path_from);
  free(jfs_path_to);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return 0;
}

static int 
jfs_chmod(const char *path, mode_t mode)
{
  char *jfs_path;
  int res;

  log_error("Called jfs_chmod, path:%s mode:%d\n", path, mode);

  jfs_path = jfs_realpath(path);
  res = chmod(jfs_path, mode);
  free(jfs_path);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return 0;
}

static int 
jfs_chown(const char *path, uid_t uid, gid_t gid)
{
  int res;

  log_error("Called jfs_chmod, path:%s uid:%d gid:%d\n", path, uid, gid);
  res = lchown(path, uid, gid);
  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return 0;
}

static int 
jfs_truncate(const char *path, off_t size)
{
  char *jfs_path;
  int res;

  log_error("Called jfs_truncate, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  res = jfs_file_truncate(jfs_path, size);
  free(jfs_path);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return 0;
}

static int 
jfs_utimens(const char *path, const struct timespec ts[2])
{
  char *jfs_path;
  int res;
  struct timeval tv[2];

  log_error("Called jfs_utimens, path:%s\n", path);
  tv[0].tv_sec = ts[0].tv_sec;
  tv[0].tv_usec = ts[0].tv_nsec / 1000;
  tv[1].tv_sec = ts[1].tv_sec;
  tv[1].tv_usec = ts[1].tv_nsec / 1000;

  jfs_path = jfs_realpath(path);
  res = utimes(jfs_path, tv);
  free(jfs_path);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return 0;
}

static int 
jfs_open(const char *path, struct fuse_file_info *fi)
{
  char *jfs_path;
  int fd;

  printf("Called jfs_file_open, path:%s, flags:%d\n", path, fi->flags);

  jfs_path = jfs_realpath(path);
  fd = jfs_file_open(jfs_path, fi->flags);
  free(jfs_path);

  if (fd < 0) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  printf("directo_io:%d, now being set to 1.\n", fi->direct_io);

  fi->fh = fd;
  fi->direct_io = 1;

  printf("Setting fi->fh:%llu\n", fi->fh);

  return 0;
}

static int 
jfs_read(const char *path, char *buf, size_t size, off_t offset,
		 struct fuse_file_info *fi)
{
  int res;

  printf("Called jfs_read, path:%s, fi->fh:%llu\n", path, fi->fh);

  res = pread(fi->fh, buf, size, offset);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    res = -errno;
  }

  printf("Pread return %d bytes in buf:%s, expected:%d\n", res, buf, size);

  return res;
}

static int 
jfs_write(const char *path, const char *buf, size_t size,
		  off_t offset, struct fuse_file_info *fi)
{
  int res;

  printf("Called jfs_write, path:%s fi->fh:%llu\n", path, fi->fh);

  res = pwrite(fi->fh, buf, size, offset);
  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    res = -errno;
  }

  return res;
}

static int 
jfs_statfs(const char *path, struct statvfs *stbuf)
{
  char *jfs_path;
  int res;

  log_error("Called jfs_statvfs, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  res = statvfs(jfs_path, stbuf);
  free(jfs_path);

  if (res == -1) {
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

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int 
jfs_setxattr(const char *path, const char *name, const char *value,
			 size_t size, int flags)
{
  char *jfs_path;
  int res;

  log_error("Called jfs_setxattr, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  res = lsetxattr(jfs_path, name, value, size, flags);
  free(jfs_path);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return 0;
}

static int
jfs_getxattr(const char *path, const char *name, char *value,
			 size_t size)
{
  char *jfs_path;
  int res;

  log_error("Called jfs_getxattr, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  res = lgetxattr(jfs_path, name, value, size);
  free(jfs_path);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return res;
}

static int 
jfs_listxattr(const char *path, char *list, size_t size)
{
  char *jfs_path;
  int res;

  log_error("Called jfs_listxattr, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  res = llistxattr(jfs_path, list, size);
  free(jfs_path);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  return res;
}

static int 
jfs_removexattr(const char *path, const char *name)
{
  char *jfs_path;
  int res;

  log_error("Called jfs_removexattr, path:%s\n", path);

  jfs_path = jfs_realpath(path);
  res = lremovexattr(jfs_path, name);
  free(jfs_path);

  if (res == -1) {
	log_error("Error occured, errno:%d\n", -errno);
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

  realpath(argv[1], jfs_context->querypath);
  jfs_context->querypath_len = strlen(jfs_context->querypath);

  realpath(argv[2], jfs_context->datapath);
  jfs_context->datapath_len = strlen(jfs_context->datapath);
  
  realpath(argv[3], jfs_context->mountpath);
  jfs_context->mountpath_len = strlen(jfs_context->mountpath);
  
  printf("querydir:%s\n", jfs_context->querypath);
  printf("datadir:%s\n", jfs_context->datapath);
  printf("mountdir:%s\n", jfs_context->mountpath);

  argc = 4;
  argv[1] = "-d";
  argv[2] = "-s";
  argv[3] = jfs_context->mountpath;

  printf("Starting joinFS, mountpath:%s\n", argv[1]);
  rc = fuse_main(argc, argv, &jfs_oper, jfs_context);;
  printf("joinFS stopped. Return code=%d.\n", rc);

  return rc;
}
