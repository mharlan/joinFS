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

#include "error_log.h"
#include "sqlitedb.h"
#include "jfs_meta.h"
#include "jfs_util.h"
#include "jfs_dir_query.h"
#include "jfs_dynamic_dir.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define JFS_FOLDER_SIMPLE_QUERY "SELECT DISTINCT m.keyvalue FROM keys AS k, metadata AS m WHERE k.keytext=\"%s\" AND m.keyid=k.keyid;"
#define JFS_FOLDER_QUERY        "SELECT DISTINCT m.keyvalue FROM keys AS k, metadata AS m WHERE k.keytext=\"%s\" AND m.keyid=k.keyid AND m.jfs_id IN (%s);"
#define JFS_FILE_QUERY          "SELECT DISTINCT l.jfs_id, l.filename, l.path FROM links AS l, metadata AS m, keys AS k WHERE l.jfs_id=m.jfs_id AND m.keyid=k.keyid AND m.jfs_id IN (%s);"

#define JFS_KEY_QUERY           "SELECT l.jfs_id FROM links AS l, metadata AS m, keys AS k WHERE l.jfs_id=m.jfs_id and m.keyid=k.keyid and k.keytext=\"%s\""
#define JFS_PAIR_QUERY          "SELECT l.jfs_id FROM links AS l, metadata AS m, keys AS k WHERE l.jfs_id=m.jfs_id and m.keyid=k.keyid and k.keytext=\"%s\" and m.keyvalue=\"%s\""
#define JFS_INTERSECT           " INTERSECT "
#define JFS_ITEM_ADJUST         2
#define JFS_PAIR_ADJUST         4

static int jfs_dir_parse_key_pairs(int skip_last, const char *dir_key_pairs, size_t *dir_current_pos, 
                                   size_t *dir_current_len, size_t *dir_query_len, char **dir_query);
static int jfs_dir_create_query(int items, int is_folder, char *path, char *dir_key_pair, char **query);
static int jfs_dir_expand_query(size_t *query_len, char **query);

int 
jfs_dir_query_builder(const char *path, const char *realpath, int *is_folders, char **query)
{
  char *copy_path;
  char *dir_is_folders;
  char *dir_key_pairs;
  char *path_items;
  char *dir_query;

  size_t path_len;

  int items;
  int rc;
  
  dir_query = NULL;
  dir_is_folders = NULL;
  dir_key_pairs = NULL;
  path_items = NULL;

  rc = jfs_meta_do_getxattr(realpath, JFS_DIR_KEY_PAIRS, &dir_key_pairs);
  if(rc) {
    return rc;
  }
  
  printf("---key_pairs, path:%s, realpath:%s\n", path, realpath);

  rc = jfs_meta_do_getxattr(realpath, JFS_DIR_IS_FOLDER, &dir_is_folders);
  if(rc) {
    rc = 0;
    *is_folders = 0;
  }
  else {
    if(strcmp(dir_is_folders, JFS_DIR_XATTR_TRUE) == 0) {
      *is_folders = 1;
    }
    else {
      *is_folders = 0;
    }
  }

  printf("---jfs_dir_is_folder\n");

  if(dir_is_folders) {
    free(dir_is_folders);
  }

  rc = jfs_meta_do_getxattr(realpath, JFS_DIR_PATH_ITEMS, &path_items);
  if(rc) {
    rc = 0;
    items = 0;
  }
  else {
    items = atoi(path_items);
  }

  printf("---jfs_dir_path_items\n");

  if(path_items) {
    free(path_items);
  }

  //copy the path so we can modify it
  path_len = strlen(path) + 1;
  copy_path = malloc(sizeof(*copy_path) * path_len);
  if(!copy_path) {
    free(dir_key_pairs);

    return -ENOMEM;
  }
  strncpy(copy_path, path, path_len);

  rc = jfs_dir_create_query(items, *is_folders, copy_path, dir_key_pairs, &dir_query);
  free(dir_key_pairs);
  free(copy_path);

  if(rc) {
    if(dir_query) {
      free(dir_query);
    }

    return rc;
  }
  
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

  rc = 0;
  datapath = NULL;

  //allocate memory
  values = malloc(sizeof(*values) * (items + 1));
  if(!values) {
    return -ENOMEM;
  }
  
  parent_key_pairs = malloc(sizeof(*parent_key_pairs) * (items + 1));
  if(!parent_key_pairs) {
    free(values);
    
    return -ENOMEM;
  }

  query_len = JFS_QUERY_INC;
  query = malloc(sizeof(*query) * query_len);
  if(!query) {
    free(values);
    free(parent_key_pairs);
    
    return -ENOMEM;
  }
  query[0] = '\0';

  //start building the query
  //get parent key_pairs and path values
  for(i = 0; i < items; ++i) {
    value = jfs_util_get_last_path_item(path);
    if(!value) {
      rc = -EBADMSG;
      if(i > 0) {
        items = i - 1;
      }
      
      goto cleanup;
    }

    *value = '\0';
    ++value;

    values[i] = value;
    
    rc = jfs_util_get_datapath(path, &datapath);
    if(rc) {
      if(i > 0) {
        items = i - 1;
      }

      goto cleanup;
    }

    printf("---datapath:%s\n", datapath);
    
    rc = jfs_meta_do_getxattr(datapath, JFS_DIR_KEY_PAIRS, &key_pairs);
    free(datapath);

    printf("----got key pairs:%s\n", key_pairs);

    if(rc) {
      if(i > 0) {
        items = i - 1;
      }

      goto cleanup;
    }

    parent_key_pairs[i] = key_pairs;
  }

  current_len = 0;
  current_pos = 0;
  for(i = 0; i < items; ++i) {
    rc = jfs_dir_parse_key_pairs(1, parent_key_pairs[i], &current_pos, &current_len, &query_len, &query);
    if(rc) {
      goto cleanup;
    }

    key = strrchr(parent_key_pairs[i], '=');
    if(!key) {
      rc = -EBADMSG;
      
      goto cleanup;
    }

    if(*(key - 1) != 'k') {
      rc = -EBADMSG;
      
      goto cleanup;
    }
    ++key;
    
    end = strlen(key) - 1;
    if(key[end] == ';') {
      key[end] = '\0';
    }
    
    value = values[i];
    current_pos = current_len;
    current_len += strlen(key) + strlen(value) + strlen(JFS_PAIR_QUERY) - JFS_PAIR_ADJUST;
    if(current_len >= query_len) {
      rc = jfs_dir_expand_query(&query_len, &query);
      if(rc) {
        goto cleanup;
      }
    }
    sprintf(&query[current_pos], JFS_PAIR_QUERY, key, value);
    current_pos = current_len;

    current_len += strlen(JFS_INTERSECT);
    if(current_len >= query_len) {
      rc = jfs_dir_expand_query(&query_len, &query);
      if(rc) {        
        goto cleanup;
      }
    }
    sprintf(&query[current_pos], JFS_INTERSECT);
    current_pos = current_len;
  }

  if(is_folders) {
    rc = jfs_dir_parse_key_pairs(1, dir_key_pairs, &current_pos, &current_len, &query_len, &query);
    if(rc) {
      goto cleanup;
    }

    key = strrchr(dir_key_pairs, '=');
    if(!key) {
      rc = -EBADMSG;
      
      goto cleanup;
    }

    if(*(key - 1) != 'k') {
      rc = -EBADMSG;
      
      goto cleanup;
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
        rc = -ENOMEM;

        goto cleanup;
      }
      snprintf(outer_query, query_len, JFS_FOLDER_SIMPLE_QUERY, key);
    }
    else {
      //remove the last intersect
      current_len -= strlen(JFS_INTERSECT);
      query[current_len] = '\0';

      query_len = strlen(JFS_FOLDER_QUERY) + strlen(key) + current_len + 1;
      outer_query = malloc(sizeof(*outer_query) * query_len);
      if(!outer_query) {
        rc = -ENOMEM;

        goto cleanup;
      }
      snprintf(outer_query, query_len, JFS_FOLDER_QUERY, key, query);
    }
  }
  else {
    rc = jfs_dir_parse_key_pairs(0, dir_key_pairs, &current_pos, &current_len, &query_len, &query);
    if(rc) {
      goto cleanup;
    }

    if(!current_len) {
      rc = -EBADMSG;
      
      goto cleanup;
    }
    else {
      //remove the last intersect
      current_len -= strlen(JFS_INTERSECT);
      query[current_len] = '\0';

      query_len = strlen(JFS_FILE_QUERY) + current_len + 1;
      outer_query = malloc(sizeof(*outer_query) * query_len);
      if(!outer_query) {
        rc = -ENOMEM;

        goto cleanup;
      }
      snprintf(outer_query, query_len, JFS_FILE_QUERY, query);
    }
  }

  *dir_query = outer_query;

 cleanup:
  for(i = 0; i < items; ++i) {
    free(parent_key_pairs[i]);
  }
  free(parent_key_pairs);
  free(values);
  free(query);
  
  return rc;
}

static int
jfs_dir_expand_query(size_t *query_len, char **query)
{
  char *new_query;

  size_t new_len;
  
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
  size_t intersect_len;
  size_t key_query_len;
  size_t pair_query_len;

  int rc;

  if(!strlen(dir_key_pairs) || dir_key_pairs[0] == ';') {
    return 0;
  }
 
  current_pos = *dir_current_pos;
  current_len = *dir_current_len;
  query_len = *dir_query_len;
  current_pos = current_len;
  query = *dir_query;

  intersect_len = strlen(JFS_INTERSECT);
  key_query_len = strlen(JFS_KEY_QUERY) - JFS_ITEM_ADJUST;
  pair_query_len = strlen(JFS_PAIR_QUERY) - JFS_PAIR_ADJUST;
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
        free(key_pairs);
  
        return -EBADMSG;
      }

      if(key != &token[1]) {
        free(key_pairs);
        
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
      free(key_pairs);
      
      return -EBADMSG;
    }
    
    //check if the next token is a value
    if(token != NULL && token[0] == 'v') {
      value = strchr(token, '=');
      if(!value) {
        free(key_pairs);
        
        return -EBADMSG;
      }

      if(value != &token[1]) {
        free(key_pairs);
        
        return -EBADMSG;
      }
      ++value;
      
      current_len += strlen(key) + strlen(value) + pair_query_len;
      if(current_len >= query_len) {
        rc = jfs_dir_expand_query(&query_len, &query);
        if(rc) {
          free(key_pairs);

          return rc;
        }
      }
      sprintf(&query[current_pos], JFS_PAIR_QUERY, key, value);
      current_pos = current_len;

      token = strtok(NULL, ";");
    }
    else {
      current_len += strlen(key) + key_query_len;
      if(current_len >= query_len) {
        rc = jfs_dir_expand_query(&query_len, &query);
        if(rc) {
          free(key_pairs);

          return rc;
        }
      }
      sprintf(&query[current_pos], JFS_KEY_QUERY, key);
      current_pos = current_len;

      //already got next token, which is not a value 
    }

    current_len += intersect_len;
    if(current_len >= query_len) {
      rc = jfs_dir_expand_query(&query_len, &query);
      if(rc) {
        free(key_pairs);
        
        return rc;
      }
    }
    sprintf(&query[current_pos], JFS_INTERSECT);
    current_pos = current_len;
  }
  free(key_pairs);

  *dir_current_pos = current_pos;
  *dir_current_len = current_len;
  *dir_query_len = query_len;
  *dir_query = query;

  return 0;
}
