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

#if !defined(_REENTRANT)
#define	_REENTRANT
#endif

#include "sqlitedb.h"
#include "jfs_meta.h"
#include "jfs_util.h"
#include "jfs_dir_query.h"
#include "jfs_dynamic_dir.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define JFS_FOLDER_SIMPLE_QUERY "SELECT DISTINCT m.keyvalue FROM keys AS k, metadata AS m WHERE k.keytext=\"%s\" AND m.keyid=k.keyid;"
#define JFS_FOLDER_QUERY        "SELECT DISTINCT m.keyvalue FROM keys AS k, metadata AS m WHERE k.keytext=\"%s\" AND m.keyid=k.keyid AND m.inode=(SELECT f.inode FROM files AS f, metadata AS m, keys AS k WHERE f.inode=m.inode and m.keyid=k.keyid and (%s));"
#define JFS_FILE_QUERY          "SELECT DISTINCT f.inode, f.filename, f.datapath FROM files AS f, metadata AS m, keys AS k WHERE f.inode=m.inode AND m.keyid=k.keyid AND (%s);"

#define JFS_SQL_OR               " OR "
#define JFS_KEY_ITEM             "(k.keytext=\"%s\") OR "
#define JFS_KEY_ITEM_ADJUST      2
#define JFS_KEY_PAIR_ITEM        "(k.keytext=\"%s\" AND m.keyvalue=\"%s\") OR "
#define JFS_KEY_PAIR_ITEM_ADJUST 4

static int jfs_dir_parse_key_pairs(int skip_last, const char *dir_key_pairs, size_t *dir_current_pos, 
                                   size_t *dir_current_len, size_t *dir_query_len, char **dir_query);
static int jfs_dir_create_query(int items, int is_folder, char *path, char *dir_key_pair, char **query);
static int jfs_dir_expand_query(size_t *query_len, char **query);

int 
jfs_dir_query_builder(const char *orig_path, const char *path, int *is_folders, char **query)
{
  char *copy_path;
  char *dir_is_folders;
  char *dir_key_pairs;
  char *path_items;
  char *dir_query;

  size_t path_len;

  int items;
  int rc;

  printf("Dynamic Path:%s, Datapath:%s\n", orig_path, path);

  rc = jfs_meta_do_getxattr(path, JFS_DIR_KEY_PAIRS, &dir_key_pairs);
  if(rc) {
    return rc;
  }
  
  rc = jfs_meta_do_getxattr(path, JFS_DIR_IS_FOLDER, &dir_is_folders);
  if(rc) {
    *is_folders = 0;
  }
  else {
    if(strcmp(dir_is_folders, JFS_DIR_XATTR_TRUE) == 0) {
      printf("---FOLDER QUERY---\n");

      *is_folders = 1;
    }
    else {
      printf("---FILE QUERY---\n");

      *is_folders = 0;
    }
  }

  rc = jfs_meta_do_getxattr(path, JFS_DIR_PATH_ITEMS, &path_items);
  if(rc) {
    items = 0;
  }
  else {
    items = atoi(path_items);
  }

  printf("--DEBUG--path_items:%s, item:%d\n", path_items, items);
  
  //copy the path so we can modify it
  path_len = strlen(orig_path) + 1;
  copy_path = malloc(sizeof(*copy_path) * path_len);
  if(!copy_path) {
    return -ENOMEM;
  }
  strncpy(copy_path, orig_path, path_len);
  
  printf("Now building query.\n");

  rc = jfs_dir_create_query(items, *is_folders, copy_path, dir_key_pairs, &dir_query);
  free(copy_path);

  if(rc) {
    printf("Query build failed.\n");
    printf("\tQuery:%s\n", dir_query);

    free(dir_query);
    return rc;
  }

  printf("Build query:%s\n", dir_query);

  *query = dir_query;

  return 0;
}

static int
jfs_dir_create_query(int items, int is_folders, char *path, char *dir_key_pairs, char **dir_query)
{
  char *datapath;
  char *key_pairs;
  char *key;
  char *value;
  char *query;
  char *outer_query;

  char **values;
  char **parent_key_pairs;

  size_t query_len;
  size_t current_len;
  size_t current_pos;
  size_t end;

  int rc;
  int i;

  //allocate memory
  values = malloc(sizeof(*values) * items);
  if(!values) {
    return -ENOMEM;
  }

  parent_key_pairs = malloc(sizeof(*parent_key_pairs) * items);
  if(!parent_key_pairs) {
    return -ENOMEM;
  }

  query_len = JFS_QUERY_INC;
  query = malloc(sizeof(*query) * query_len);
  if(!query) {
    return -ENOMEM;
  }
  query[0] = '\0';

  //start building the query
  //get parent key_pairs and path values
  for(i = 0; i < items; ++i) {
    value = jfs_util_get_last_path_item(path);
    if(!value) {
      printf("Failed to get path value.\n");
      free(query);
      return -EBADMSG;
    }

    *value = '\0';
    ++value;

    values[i] = value;

    rc = jfs_util_get_datapath(path, &datapath);
    if(rc) {
      free(query);
      return rc;
    }

    rc = jfs_meta_do_getxattr(datapath, JFS_DIR_KEY_PAIRS, &key_pairs);
    if(rc) {
      free(query);
      return rc;
    }

    parent_key_pairs[i] = key_pairs;
  }

  current_len = 0;
  current_pos = 0;
  for(i = 0; i < items; ++i) {
    rc = jfs_dir_parse_key_pairs(1, parent_key_pairs[i], &current_pos, &current_len, &query_len, &query);
    if(rc) {
      free(query);
      return rc;
    }

    key = strrchr(parent_key_pairs[i], '=');
    if(!key) {
      free(query);
      return -EBADMSG;
    }

    if(*(key - 1) != 'k') {
      free(query);
      return -EBADMSG;
    }
    ++key;
    
    end = strlen(key) - 1;
    if(key[end] == ';') {
      key[end] = '\0';
    }

    printf("Current_pos:%d, Current_len:%d, strlen(query):%d\n", current_pos, current_len, strlen(query));

    value = values[i];
    current_pos = current_len;
    current_len += strlen(key) + strlen(value) + strlen(JFS_KEY_PAIR_ITEM) - JFS_KEY_PAIR_ITEM_ADJUST;
    if(current_len >= query_len) {
      rc = jfs_dir_expand_query(&query_len, &query);
      if(rc) {
        free(query);
        return rc;
      }
    }
    sprintf(&query[current_pos], JFS_KEY_PAIR_ITEM, key, value);
    current_pos = current_len;

    printf("--DEBUG--Query_pairs after:%s\n", query);
  }

  if(is_folders) {
    rc = jfs_dir_parse_key_pairs(1, dir_key_pairs, &current_pos, &current_len, &query_len, &query);
    if(rc) {
      free(query);
      return rc;
    }

    key = strrchr(dir_key_pairs, '=');
    if(!key) {
      free(query);
      return -EBADMSG;
    }

    if(*(key - 1) != 'k') {
      free(query);
      return -EBADMSG;
    }
    ++key;
    
    end = strlen(key) - 1;
    if(key[end] == ';') {
      key[end] = '\0';
    }

    if(!current_len) {
      query_len = strlen(JFS_FOLDER_SIMPLE_QUERY) + strlen(key) + 1;
      outer_query = malloc(sizeof(*outer_query) * query_len);
      if(!outer_query) {
        free(query);
        return -ENOMEM;
      }
      snprintf(outer_query, query_len, JFS_FOLDER_SIMPLE_QUERY, key);
    }
    else {
      //remove the last OR

      printf("strlen(query):%d, Current_len before:%d\n", strlen(query), current_len);
      printf("Query:%s\n", query);

      current_len -= strlen(JFS_SQL_OR);
      query[current_len] = '\0';

      printf("Current_len after:%d\n", current_len);

      query_len = strlen(JFS_FOLDER_QUERY) + strlen(key) + current_len + 1;
      outer_query = malloc(sizeof(*outer_query) * query_len);
      if(!outer_query) {
        free(query);
        return -ENOMEM;
      }
      snprintf(outer_query, query_len, JFS_FOLDER_QUERY, key, query);
    }
  }
  else {
    rc = jfs_dir_parse_key_pairs(0, dir_key_pairs, &current_pos, &current_len, &query_len, &query);
    if(rc) {
      free(query);
      return rc;
    }

    if(!current_len) {
      free(query);
      return -EBADMSG;
    }
    else {
      //remove the last OR
      current_len -= strlen(JFS_SQL_OR);
      query[current_len] = '\0';

      query_len = strlen(JFS_FILE_QUERY) + current_len + 1;
      outer_query = malloc(sizeof(*outer_query) * query_len);
      if(!outer_query) {
        free(query);
        return -ENOMEM;
      }
      snprintf(outer_query, query_len, JFS_FILE_QUERY, query);
    }
  }

  free(query);
  *dir_query = outer_query;
  
  return 0;
}

static int
jfs_dir_expand_query(size_t *query_len, char **query)
{
  char *new_query;

  size_t new_len;

  printf("Expanding query.\n");

  new_len = *query_len + JFS_QUERY_INC;
  new_query = malloc(sizeof(*new_query) * new_len);
  if(!new_query) {
    return -ENOMEM;
  }
  strncpy(new_query, *query, *query_len);
  free(*query);

  *query_len = new_len;
  *query = new_query;

  return 0;
}

static int
jfs_dir_parse_key_pairs(int skip_last, const char *dir_key_pairs, size_t *dir_current_pos,
                        size_t *dir_current_len, size_t *dir_query_len, char **dir_query)
{
  char *key;
  char *value;
  char *query;
  char *token;
  char *key_pairs;

  size_t current_pos;
  size_t current_len;
  size_t query_len;
  size_t key_pairs_len;
  size_t jfs_sql_or_len;
  size_t key_item_len;
  size_t key_pair_item_len;

  int rc;

  if(!strlen(dir_key_pairs) || dir_key_pairs[0] == ';') {
    return 0;
  }
 
  current_pos = *dir_current_pos;
  current_len = *dir_current_len;
  query_len = *dir_query_len;
  current_pos = current_len;
  query = *dir_query;

  jfs_sql_or_len = strlen(JFS_SQL_OR);
  key_item_len = strlen(JFS_KEY_ITEM) - JFS_KEY_ITEM_ADJUST;
  key_pair_item_len = strlen(JFS_KEY_PAIR_ITEM) - JFS_KEY_PAIR_ITEM_ADJUST;
  key_pairs_len = strlen(dir_key_pairs) + 1;

  key_pairs = malloc(sizeof(*key_pairs) * key_pairs_len);
  if(!key_pairs) {
    return -ENOMEM;
  }
  strncpy(key_pairs, dir_key_pairs, key_pairs_len);

  //parse the key value pairs
  token = strtok(key_pairs, ";");
  while(token != NULL) {
    //get the key
    if(token[0] == 'k') {
      key = strchr(token, '=');
      if(!key) {
        printf("Failed to extract key from key pair.\n");

        return -EBADMSG;
      }

      if(key != &token[1]) {
        return -EBADMSG;
      }
      ++key;

      //last token, don't add to query, will be used later
      token = strtok(NULL, ";");
      if(skip_last && token == NULL) {
        break;
      }
    }
    //must be a key before there is a value
    else {
      return -EBADMSG;
    }
    
    //check if the next token is a value
    if(token != NULL && token[0] == 'v') {
      value = strchr(token, '=');
      if(!value) {
        printf("Failed to get value from key pair.\n");

        return -EBADMSG;
      }

      if(value != &token[1]) {
        return -EBADMSG;
      }
      ++value;

      printf("Current_pos:%d, current_len:%d, strlen(query):%d\n", current_pos, current_len, strlen(query));
      printf("Adding key_pair key:%s, value:%s\n", key, value);
      
      current_len += strlen(key) + strlen(value) + key_pair_item_len;
      if(current_len >= query_len) {
        rc = jfs_dir_expand_query(&query_len, &query);
        if(rc) {
          return rc;
        }
      }
      sprintf(&query[current_pos], JFS_KEY_PAIR_ITEM, key, value);
      current_pos = current_len;

      token = strtok(NULL, ";");
    }
    else {
      printf("Current_pos:%d, current_len:%d, strlen(query):%d\n", current_pos, current_len, strlen(query));
      printf("Adding key:%s\n", key);
      
      current_len += strlen(key) + key_item_len;
      if(current_len >= query_len) {
        rc = jfs_dir_expand_query(&query_len, &query);
        if(rc) {
          return rc;
        }
      }
      sprintf(&query[current_pos], JFS_KEY_ITEM, key);
      current_pos = current_len;

      //already got next token, which is not a value 
    }
  }

  *dir_current_pos = current_pos;
  *dir_current_len = current_len;
  *dir_query_len = query_len;
  *dir_query = query;

  return 0;
}
