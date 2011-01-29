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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <attr/xattr.h>

#define LIST_MAX 1024
#define TEST_FILE "/home/joinfs/git/joinFS/demo/mount/test.mp3"

int
main()
{
  struct stat buf;
  char *buffer;
  size_t buff_size;
  int rc;
  size_t i;
  
  buffer = malloc(sizeof(*buffer) * LIST_MAX);
  if(!buffer) {
	printf("Failed to allocate memory for buffer.\n");
	return -1;
  }

  printf("--TESTING GETATTR FOR FILE:%s\n", TEST_FILE);
  rc = stat(TEST_FILE, &buf);
  if(rc) {
	printf("--ERROR CODE:%d, returned, getattr test failed.\n", -errno);
  }

  rc = setxattr(TEST_FILE, "attr1", "hello", 5, XATTR_CREATE);
  if(rc) {
	printf("--ERROR CODE:%d, returned\n", -errno);
  }
  rc = setxattr(TEST_FILE, "attr2", "world", 5, XATTR_CREATE);
  if(rc) {
	printf("--ERROR CODE:%d, returned\n", -errno);
  }
  rc = setxattr(TEST_FILE, "media codec", "mpeg 3", 6, XATTR_CREATE);
  if(rc) {
	printf("--ERROR CODE:%d, returned\n", -errno);
  }
  rc = setxattr(TEST_FILE, "artist", "Grizzly Bear", 12, XATTR_CREATE);
  if(rc) {
	printf("--ERROR CODE:%d, returned\n", -errno);
  }

  buff_size = listxattr(TEST_FILE, buffer, LIST_MAX);
  if(buff_size > LIST_MAX) {
	free(buffer);
	buffer = malloc(sizeof(*buffer) * buff_size);
	buff_size = listxattr(TEST_FILE, buffer, buff_size);
  }
  else if(buff_size == 0) {
	printf("--ERROR CODE:%d, returned\n", -errno);
  }
  else {
	printf("Listxattr result for path:%s\nBuffer Size:%d\n", TEST_FILE, buff_size);
	for(i = 0; i < buff_size; ++i) {
	  if(buffer[i] == '\0') {
		printf("\\0");
	  }
	  else printf("%c", buffer[i]);
	}
	printf("\n");
  }
	
  rc = getxattr(TEST_FILE, "attr1", buffer, buff_size);
  if(rc < 0) {
	printf("--ERROR CODE:%d, returned\n", -errno);
  }
  printf("attr1:%s\n", buffer);
  rc = getxattr(TEST_FILE, "attr2", buffer, buff_size);
  if(rc < 0) {
	printf("--ERROR CODE:%d, returned\n", -errno);
  }
  printf("attr2:%s\n", buffer);
  rc = getxattr(TEST_FILE, "media codec", buffer, buff_size);
  if(rc < 0) {
	printf("--ERROR CODE:%d, returned\n", -errno);
  }
  printf("media codec:%s\n", buffer);
  rc = getxattr(TEST_FILE, "artist", buffer, buff_size);
  if(rc < 0) {
	printf("--ERROR CODE:%d, returned\n", -errno);
  }
  printf("artist:%s\n", buffer);

  rc = setxattr(TEST_FILE, "attr1", "nandos is tasty", 15, XATTR_REPLACE);
  if(rc) {
	printf("--ERROR CODE:%d, returned\n", -errno);
  }
  rc = getxattr(TEST_FILE, "attr1", buffer, buff_size);
  if(rc < 0) {
	printf("--ERROR CODE:%d, returned\n", -errno);
  }
  printf("New attr1:%s\n", buffer);

  rc = setxattr(TEST_FILE, "attr1", "shouldn't see this", 18, XATTR_CREATE);
  if(rc) {
	printf("--(SUCCESS!)ERROR CODE:%d, returned\n", -errno);
  }
  rc = getxattr(TEST_FILE, "attr1", buffer, buff_size);
  if(rc < 0) {
	printf("--ERROR CODE:%d, returned\n", -errno);
  }
  printf("attr1 again:%s\n", buffer);

  return 0;
}
