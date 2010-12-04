CC=gcc
INCLUDE=-I include/
CFLAGS=$(INCLUDE) -D_FILE_OFFSET_BITS=64 -Wall -o
SQLFLAGS=-W -O2 -Wl,-R/usr/local/lib -lsqlite3
THRDFLAGS=-lpthread
TIMEFLAGS=-lrt
FUSEFLAGS=`pkg-config fuse --cflags --libs`
STDFLAGS=$(TIMEFLAGS) $(THRDFLAGS) $(SQLFLAGS) $(CFLAGS)
ALLFLAGS=$(FUSEFLAGS) $(STDFLAGS)
OBJS1=obj/error_log.o obj/sqlitedb.o obj/thr_pool.o obj/result.o obj/jfs_list.o
OBJS2=obj/jfs_file.o obj/jfs_file_cache.o
OBJS=$(OBJS1) $(OBJS2)
TESTS=tests/jfs_pool_test

all: joinfs $(TESTS)

joinfs: $(OBJS)
	$(CC) -g -O0 $(ALLFLAGS) joinfs joinfs.c $(OBJS)

tests/jfs_pool_test: $(OBJS)
	$(CC) -g -O0 $(STDFLAGS) tests/jfs_pool_test tests/jfs_pool_test.c $(OBJS)

obj/jfs_file_cache.o:
	$(CC) -c $(CFLAGS) obj/jfs_file_cache.o jfs_file_cache.c

obj/jfs_file.o:
	$(CC) -c $(FUSEFLAGS) $(CFLAGS) obj/jfs_file.o jfs_file.c

obj/result.o: 
	$(CC) -c $(CFLAGS) obj/result.o result.c

obj/thr_pool.o:
	$(CC) -c $(CFLAGS) obj/thr_pool.o thr_pool.c

obj/sqlitedb.o:
	$(CC) -c $(CFLAGS) obj/sqlitedb.o sqlitedb.c

obj/jfs_list.o:
	$(CC) -c $(CFLAGS) obj/jfs_list.o jfs_list.c

obj/error_log.o:
	$(CC) -c $(CFLAGS) obj/error_log.o error_log.c 

clean:
	rm -rf obj/*.o tests/jfs_pool_test joinfs
