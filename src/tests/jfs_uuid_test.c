#include "jfs_uuid.h"

#include <stdio.h>

/*
 * Tests UUID generation.
 */
int main()
{
  char *uuid;
  int i;

  printf("JoinFS UUID test start.\n");

  uuid = jfs_create_uuid();

  for(i = 0; i < 100; ++i) {
	jfs_generate_uuid(uuid);
	printf("UUID(%d):%s\n", i, uuid);
  }

  jfs_destroy_uuid(uuid);

  return 0;
}
