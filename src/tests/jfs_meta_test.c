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
#include <attr/xattr.h>

#define LIST_MAX 1024
#define TEST_FILE "/home/joinfs/git/joinFS/demo/mount/test.mp3"

int
main()
{
  char *buffer;
  size_t buff_size;
  int i;
  
  buffer = malloc(sizeof(*buffer) * LIST_MAX);
  if(!buffer) {
	printf("Failed to allocate memory for buffer.\n");
	return -1;
  }

  setxattr(TEST_FILE, "attr1", "hello", 5, XATTR_CREATE);
  setxattr(TEST_FILE, "attr2", "world", 5, XATTR_CREATE);
  setxattr(TEST_FILE, "media codec", "mpeg 3", 6, XATTR_CREATE);
  setxattr(TEST_FILE, "artist", "Grizzly Bear", 12, XATTR_CREATE);

  buff_size = listxattr(TEST_FILE, buffer, LIST_MAX);
  if(buff_size > LIST_MAX) {
	free(buffer);
	buffer = malloc(sizeof(*buffer) * buff_size);
	buff_size = listxattr(TEST_FILE, buffer, buff_size);
  }

  printf("Listxattr result for path:%s\nBuffer Size:%d\n", TEST_FILE, buff_size);
  for(i = 0; i < buff_size; ++i) {
	if(buffer[i] == '\0') {
	  printf("\\0");
	}
	else printf("%c", buffer[i]);
  }
  printf("\n");

  getxattr(TEST_FILE, "attr1", buffer, buff_size);
  printf("attr1:%s\n", buffer);
  getxattr(TEST_FILE, "attr2", buffer, buff_size);
  printf("attr2:%s\n", buffer);
  getxattr(TEST_FILE, "media codec", buffer, buff_size);
  printf("media codec:%s\n", buffer);
  getxattr(TEST_FILE, "artist", buffer, buff_size);
  printf("artist:%s\n", buffer);

  return 0;
}
