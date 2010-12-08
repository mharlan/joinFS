#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define JFS_MPATH     "/home/joinfs/git/joinFS/demo/.queries"
#define JFS_MPATH_LEN 37

static char *
jfs_realpath(const char *path)
{
  char *jfs_realpath;
  int path_len;

  printf("Called jfs_realpath, path:%s\n", path);

  path_len = strlen(path) + JFS_MPATH_LEN + 2;
  jfs_realpath = malloc(sizeof(*jfs_realpath) * path_len);
  if(!jfs_realpath) {
	printf("Failed to allocate memory for jfs_realpath.\n");
  }
  else {
	if(path[0] == '/') {
	  snprintf(jfs_realpath, path_len, "%s%s", JFS_MPATH, path);
	}
	else if(path == NULL) {
	  snprintf(jfs_realpath, path_len, "%s/", JFS_MPATH);
	}
	else {
	  snprintf(jfs_realpath, path_len, "%s/%s", JFS_MPATH, path);
	}

	printf("Computed realpath:%s\n", jfs_realpath);
  }

  return jfs_realpath;
}

int main()
{
  char *jfs_path;

  printf("Started joinFS realpath test.\n");

  jfs_path = jfs_realpath("/");
  free(jfs_path);

  jfs_path = jfs_realpath("");
  free(jfs_path);

  printf("Finished joinFS realpath tests.\n");

  return 0;
}
