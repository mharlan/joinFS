#ifndef JOINFS_RESULT_H
#define JOINFS_RESULT_H

#define JFS_FILENAME_MAX 1024
#define JFS_KEY_MAX      1024
#define JFS_VALUE_MAX    1024

/*
 * SGLIB list header file macros.
 */
#define JFS_LIST_CMP(a, b) (a->inode - b->inode)

SGLIB_DEFINE_LIST_PROTOTYPES(jfs_list_t, JFS_LIST_CMP, next)

/*
 * Enumerator for jfs result types.
 */
enum jfs_t {
  jfs_s_file,
  jfs_d_file,
  jfs_s_folder,
  jfs_d_folder,
  jfs_search
};

/*
 * The jfs database result type.
 */
typedef struct jfs_list jfs_list_t;
struct jfs_list {
  jfs_list_t *next;
  int         inode;
  char       *filename;
  char       *key;
  char       *value;
};

/*
 * Function to return the result of a query.
 * The result will be set based on the jfs_t.
 */
int jfs_db_result(struct jfs_db_op *db_op);

#endif
