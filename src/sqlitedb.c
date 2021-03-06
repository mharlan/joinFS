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

#include "jfs_list.h"
#include "error_log.h"
#include "sqlitedb.h"
#include "result.h"
#include "joinfs.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sqlite3.h>
#include <pthread.h>

#define ERR_MAX 256

//does not need to be public, prepares queries
static int setup_stmt(sqlite3 *db, sqlite3_stmt **stmt, const char* query);

void
jfs_init_db(void)
{
  int rc;

  /* start sqlite in multithreaded mode */
  rc = sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
  if(rc != SQLITE_OK) {
	log_error("Failed to configure multithreading for SQLITE.\n");
    log_destroy();

	exit(EXIT_FAILURE);
  }

  rc = sqlite3_initialize();
  if(rc != SQLITE_OK) {
	log_error("Failed to initialize SQLite.\n");
    log_destroy();

	exit(EXIT_FAILURE);
  }

  log_msg("SQLite started.\n");
}

/*
 * Creates a database operation.
 */
int
jfs_db_op_create(struct jfs_db_op **op, enum jfs_db_ops jfs_op, const char *format, ...)
{
  struct jfs_db_op *db_op;
  
  va_list args;

  char *query;

  int query_size;
  int rc;

  rc = 0;
  query = NULL;
  query_size = JFS_QUERY_INC;

  db_op = malloc(sizeof(*db_op));
  if(!db_op) {
    return -ENOMEM;
  }
  
  query = malloc(sizeof(*query) * query_size);
  if(!query) {
    free(db_op);
    
    return -ENOMEM;
  }
  
  while(1) {
    va_start(args, format);
    rc = vsnprintf(query, query_size, format, args);
    va_end(args);
    
    if(rc > -1 && rc < query_size) {
      break;
    }
    else if(rc > -1) {
      query_size = rc + 1;
    }
    else {
      if(errno == EILSEQ) {
        free(db_op);
        
        return -errno;
      }
      
      query_size += JFS_QUERY_INC;
    }
    
    free(query);
    query = malloc(sizeof(*query) * query_size);
    if(!query) {
      free(db_op);
      
      return -ENOMEM;
    }
  }
  
  db_op->op = jfs_op;
  db_op->query = query;
  db_op->multi_query = NULL;
  db_op->db = NULL;
  db_op->stmt = NULL;
  db_op->result = NULL;
  db_op->done = 0;
  db_op->rc = 0;

  pthread_cond_init(&db_op->cond, NULL);
  pthread_mutex_init(&db_op->mut, NULL);

  *op = db_op;

  return 0;
}

/*
 * Create a multi-write operation.
 */
int jfs_db_op_create_multi_op(struct jfs_db_op **op, int num_queries,...)
{
  struct jfs_db_op *db_op;

  char *query;

  va_list args;

  int i;

  db_op = malloc(sizeof(*db_op));
  if(!db_op) {
    return -ENOMEM;
  }

  db_op->multi_query = malloc(sizeof(*db_op->multi_query) * num_queries);
  if(!db_op->multi_query) {
    free(db_op);
    return -ENOMEM;
  }

  va_start(args, num_queries);
  for(i = 0; i < num_queries; ++i) {
    query = va_arg(args, char *);
    if(!query) {
      free(db_op);
      return -EINVAL;
    }

    db_op->multi_query[i] = query;
  }

  db_op->op = jfs_multi_write_op;
  db_op->query = NULL;
  db_op->db = NULL;
  db_op->stmt = NULL;
  db_op->result = NULL;
  db_op->num_queries = num_queries;
  db_op->done = 0;
  db_op->rc = 0;

  pthread_cond_init(&db_op->cond, NULL);
  pthread_mutex_init(&db_op->mut, NULL);

  *op = db_op;

  return 0;
}

/*
 * Create a db query using a format string.
 */
int jfs_db_op_create_query(char **query, const char *format, ...)
{
  va_list args;

  char *new_query;

  int query_size;
  int rc;

  rc = 0;
  new_query = NULL;
  query_size = JFS_QUERY_INC;
  
  new_query = malloc(sizeof(*new_query) * query_size);
  if(!new_query) {
    return -ENOMEM;
  }

  while(1) {
    va_start(args, format);
    rc = vsnprintf(new_query, query_size, format, args);
    va_end(args);
    
    if(rc > -1 && rc < query_size) {
      break;
    }
    else if(rc > -1) {
      query_size = rc + 1;
    }
    else {
      if(errno == EILSEQ) {
        free(new_query);

        return -errno;
      }
      
      query_size += JFS_QUERY_INC;
    }
    
    free(new_query);
    new_query = malloc(sizeof(*new_query) * query_size);
    if(!query) {
      return -ENOMEM;
    }
  }

  *query = new_query;

  return 0;
}

/*
 * Creates a database operation.
 */
int
jfs_do_db_op_create(struct jfs_db_op **op, enum jfs_db_ops jfs_op, char *query)
{
  struct jfs_db_op *db_op;

  db_op = malloc(sizeof(*db_op));
  if(!db_op) {
	free(query);
	return -ENOMEM;
  }

  db_op->op = jfs_op;
  db_op->query = query;
  db_op->multi_query = NULL;
  db_op->db = NULL;
  db_op->stmt = NULL;
  db_op->result = NULL;
  db_op->num_queries = 0;
  db_op->done = 0;
  db_op->rc = 0;

  pthread_cond_init(&db_op->cond, NULL);
  pthread_mutex_init(&db_op->mut, NULL);

  *op = db_op;

  return 0;
}

/*
 * Destroy a database operation.
 */
void
jfs_db_op_destroy(struct jfs_db_op *db_op)
{
  int i;

  if(!db_op->rc) {
	switch(db_op->op) {
    case(jfs_datapath_cache_op):
      free(db_op->result->datapath);
      free(db_op->result);
      break;
	case(jfs_key_cache_op):
	  free(db_op->result);
	  break;
	case(jfs_meta_cache_op):
	  free(db_op->result->value);
	  free(db_op->result);
	  break;
	case(jfs_listattr_op):
	  jfs_list_destroy(db_op->result, jfs_listattr_op);
	  break;
	case(jfs_dynamic_file_op):
	  free(db_op->result);
	  break;
	default:
	  break;
	}
  }

  if(db_op->op == jfs_multi_write_op) {
    for(i = 0; i < db_op->num_queries; ++i) {
      free(db_op->multi_query[i]);
    }
    free(db_op->multi_query);
  }
  else {
    free(db_op->query);
  }

  free(db_op);
}

/*
 * Performs a database operation that blocks while waiting
 * for the query result.
 *
 * Returns the size of the result and an error code if the
 * query failed.
 */
int
jfs_db_op_wait(struct jfs_db_op *db_op)
{
  pthread_mutex_lock(&db_op->mut);
  while(!db_op->done) {
	pthread_cond_wait(&db_op->cond, &db_op->mut);
  }
  pthread_mutex_unlock(&db_op->mut);

  return db_op->rc;
}

/*
 * Open a connection to joinfs.db
 */
int
jfs_open_db(sqlite3 **db, int sqlite_attr)
{
  sqlite3 *new_db;

  int rc;
  
  rc = sqlite3_open_v2(joinfs_context.dbpath, &new_db, sqlite_attr, NULL);
  if(rc) {
    log_error("Failed to open database file at: %s\n", joinfs_context.dbpath);
    if(new_db) {
      sqlite3_close(new_db);
    }
    
    return 0;
  }
  
  rc = sqlite3_exec(new_db, "PRAGMA journal_mode=truncate;", NULL, NULL, NULL);
  if(rc) {
    sqlite3_close(new_db);

    return rc;
  }
  
  rc = sqlite3_exec(new_db, "PRAGMA synchronous=OFF;", NULL, NULL, NULL);
  if(rc) {
    sqlite3_close(new_db);

    return rc;
  }

  rc = sqlite3_exec(new_db, "PRAGMA temp_store=MEMORY;", NULL, NULL, NULL);
  if(rc) {
    sqlite3_close(new_db);

    return rc;
  }

  *db = new_db;
  
  return 0;
}

/*
 * Close a database connection.
 */
void 
jfs_close_db(sqlite3 *db)
{
  sqlite3_close(db);
}

/*
 * Setup a sqlite prepared statement.
 */
static int
setup_stmt(sqlite3 *db, sqlite3_stmt **stmt, const char* query)
{
  int rc;

  rc = sqlite3_prepare_v2(db, query, strlen(query), stmt, NULL);
  if(rc != SQLITE_OK) {
    return rc;
  }

  return 0;
}

/*
 * Perform a query.
 */
int
jfs_query(struct jfs_db_op *db_op)
{
  sqlite3_stmt *stmt;
  
  int rc;
  int i;

  if(db_op->op == jfs_multi_write_op) {
    rc = sqlite3_exec(db_op->db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    if(rc) {
      return rc;
    }

    for(i = 0; i < db_op->num_queries; ++i) {
      rc = setup_stmt(db_op->db, &stmt, db_op->multi_query[i]);
      if(rc) {
        return rc;
      }

      db_op->stmt = stmt;
      rc = jfs_db_result(db_op);
      if(rc) {
        sqlite3_exec(db_op->db, "ROLLBACK TRANSACTION;", NULL, NULL, NULL);
        return rc;
      }
    }

    rc = sqlite3_exec(db_op->db, "COMMIT TRANSACTION;", NULL, NULL, NULL);
    if(rc) {
      return rc;
    }
  }
  else {
    rc = setup_stmt(db_op->db, &stmt, db_op->query);
    if(rc) {
      return rc;
    }
    
    db_op->stmt = stmt;
    rc = jfs_db_result(db_op);
    if(rc) {
      return rc;
    }
  }

  return 0;
}
