#ifndef JFS_THR_POOL_H
#define JFS_THR_POOL_H

/************************************************************************
 * Copyright 2008 Sun Microsystems
 *
 * Modifications by Matthew Harlan <mharlan@gwmail.gwu.edu>
 *
 * Code modified from the Sun Multithreaded Programming Guide's
 * extended example section on thread pool programming and is 
 * available here:
 *
 * (pdf) http://dlc.sun.com/pdf/816-5137/816-5137.pdf
 * (webpage) http://docs.sun.com/app/docs/doc/816-5137/ggedd?l=en&a=view
 *
 * Without an explicit license, this code is distrbuted under the GNU 
 * General Public License as modified for use in joinFS.
 ************************************************************************/

#include "sqlitedb.h"

#include <pthread.h>

typedef unsigned int uint_t;

/*!
 * The thr_pool_t type is opaque to the client.
 * It is created by thr_pool_create() and must be passed
 * unmodified to the remainder of the interfaces.
 */
typedef	struct thr_pool	thr_pool_t;

/*!
 * Create a thread pool.
 * \param min_threads The minimum number of threads kept in the pool, always available to perform work requests.
 * \param max_threads The maximum number of threads that can be in the pool, performing work requests.
 * \param linger The number of seconds excess idle worker threads (greater than min_threads) linger before exiting.
 * \param attr Attributes of all worker threads (can be NULL). Can be destroyed after calling thr_pool_create().
 * \return On error, thr_pool_create() returns NULL with errno set to the error code.
 */
thr_pool_t *jfs_pool_create(uint_t min_threads, uint_t max_threads,
							uint_t linger, pthread_attr_t *attr, int sqlite_attr);

/*!
 * Enqueue a work request to the thread pool job queue.
 * If there are idle worker threads, awaken one to perform the job.
 * Else if the maximum number of workers has not been reached,
 * create a new worker thread to perform the job.
 * Else just return after adding the job to the queue;
 * an existing worker thread will perform the job when
 * it finishes the job it is currently performing.
 *
 * The job is performed as if a new detached thread were created for it:
 *	pthread_create(NULL, attr, void *(*func)(void *), void *arg);
 * \param pool The thread pool.
 * \param db_op The database operation.
 * \return On error, thr_pool_queue() returns -1 with errno set to the error code.
 */
int	jfs_pool_queue(thr_pool_t *pool, struct jfs_db_op *db_op);

/*!
 * Wait for all queued jobs to complete.
 * \param pool The thread pool.
 */
void jfs_pool_wait(thr_pool_t *pool);

/*!
 * Cancel all queued jobs and destroy the pool.
 * \param pool The thread pool.
 */
void jfs_pool_destroy(thr_pool_t *pool);

#endif
