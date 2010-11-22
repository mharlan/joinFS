CC=gcc
INCLUDE=-I include/
CFLAGS=-o
SQLFLAGS=-Wall -W -O2 -Wl,-R/usr/local/lib -lsqlite3
THRDFLAGS=-lpthread
TIMEFLAGS=-lrt
ALLFLAGS=$(TIMEFLAGS) $(THRDFLAGS) $(SQLFLAGS) $(CFLAGS)
OBJS=obj/error_log.o obj/sqlitedb.o obj/thr_pool.o obj/result.o obj/jfs_list.o

all: jfs_pool_test

jfs_pool_test: $(OBJS)
	$(CC) -g -O0 $(INCLUDE) $(ALLFLAGS) tests/jfs_pool_test tests/jfs_pool_test.c $(OBJS)

obj/result.o: 
	$(CC) -c $(INCLUDE) $(CFLAGS) obj/result.o result.c

obj/thr_pool.o:
	$(CC) -c $(INCLUDE) $(CFLAGS) obj/thr_pool.o thr_pool.c

obj/sqlitedb.o:
	$(CC) -c $(INCLUDE) $(CFLAGS) obj/sqlitedb.o sqlitedb.c

obj/jfs_list.o:
	$(CC) -c $(INCLUDE) $(CFLAGS) obj/jfs_list.o jfs_list.c

obj/error_log.o:
	$(CC) -c $(INCLUDE) $(CFLAGS) obj/error_log.o error_log.c 

clean:
	rm -rf obj/*.o tests/jfs_pool_test