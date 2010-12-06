#include "error_log.h"
#include "jfs_uuid.h"

#include <stdlib.h>
#include <uuid/uuid.h>

/*
 * Allocate the memory to store a uuid.
 */
char *
jfs_create_uuid()
{
  char *uuid;

  uuid = malloc(sizeof(*uuid) * JFS_UUID_LEN);
  if(!uuid) {
	log_error("Failed to allocate memory for new uuid.\n");
  }

  return uuid;
}

/*
 * Deallocate the memory for a uuid.
 */
void
jfs_destroy_uuid(char *uuid)
{
  free(uuid);
}

/*
 * Generates a new unique joinFS id.
 *
 * Memory for the uuid must already be allocated with
 * jfs_create_uuid().
 */
void
jfs_generate_uuid(char *uuid)
{
  uuid_t id;

  uuid_generate(id);
  uuid_unparse_lower(id, uuid);
}
