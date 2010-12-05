#ifndef JOINFS_JFS_UUID_H
#define JOINFS_JFS_UUID_H

/*
 * Allocate the memory to store a uuid.
 */
char *jfs_create_uuid();

/*
 * Deallocate the memory for a uuid.
 */
void jfs_destroy_uuid(char *uuid);

/*
 * Generates a new unique joinFS id.
 *
 * Memory for the uuid must already be allocated with
 * jfs_create_uuid().
 */
void jfs_generate_uuid(char *uuid);

#endif
