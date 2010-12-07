#ifndef JOINFS_JFS_LIST_H
#define JOINFS_JFS_LIST_H
/*
 * joinFS: Header for jfs_list_t code.
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 */

#include "sglib.h"

/*
 * Enumerator used to determine what fields 
 * are set in a jfs_list node.
 */
enum jfs_t {
  jfs_write_op,
  jfs_datapath_op,
  jfs_s_file,
  jfs_d_file,
  jfs_s_folder,
  jfs_d_folder,
  jfs_search
};

/*
 * The jfs_list node. Used for storing
 * a database result row.
 */
typedef struct jfs_list jfs_list_t;
struct jfs_list {
  jfs_list_t *next;
  int         inode;
  char       *datapath;
  char       *filename;
  char       *key;
  char       *value;
};

/*
 * SGLIB list header file macros.
 */
#define JFS_LIST_CMP(a, b) (a->inode - b->inode)
SGLIB_DEFINE_LIST_PROTOTYPES(jfs_list_t, JFS_LIST_CMP, next)

#endif
