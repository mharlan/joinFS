CC=gcc
INCLUDE=-I include/
CFLAGS=$(INCLUDE) -D_FILE_OFFSET_BITS=64 -Wall -o
SQL=-W -O2 -Wl,-R/usr/local/lib -lsqlite3
THRDS=-lpthread
TIME=-lrt
UUID=/lib/libuuid.so.1.3.0
FUSE=`pkg-config fuse --cflags --libs` -lfuse
STDFLAGS=$(TIME) $(THRDS) $(SQL) $(CFLAGS)
ALLFLAGS=$(FUSE) $(UUID) $(STDFLAGS)
OBJS1=obj/error_log.o obj/sqlitedb.o obj/thr_pool.o obj/result.o obj/jfs_list.o
OBJS2=obj/jfs_file.o obj/jfs_file_cache.o obj/joinfs.o obj/jfs_uuid.o
OBJS=$(OBJS1) $(OBJS2)
TESTS=tests/jfs_pool_test tests/jfs_uuid_test tests/jfs_realpath_test

all: ../demo/joinfs

tests: $(OBJS) $(TESTS)

../demo/joinfs: $(OBJS)
	$(CC) -ggdb -O0 $(ALLFLAGS) ../demo/joinfs $(OBJS)

tests/jfs_realpath_test:
	$(CC) -ggdb -O0 $(CFLAGS) tests/jfs_realpath_test tests/jfs_realpath_test.c

tests/jfs_uuid_test: obj/jfs_uuid.o obj/error_log.o
	$(CC) -ggdb -O0 $(UUID) $(CFLAGS) tests/jfs_uuid_test tests/jfs_uuid_test.c obj/jfs_uuid.o obj/error_log.o

tests/jfs_pool_test: $(OBJS1)
	$(CC) -ggdb -O0 $(STDFLAGS) tests/jfs_pool_test tests/jfs_pool_test.c $(OBJS1)

obj/jfs_uuid.o:
	$(CC) -c $(CFLAGS) obj/jfs_uuid.o jfs_uuid.c

obj/joinfs.o:
	$(CC) -c $(CFLAGS) obj/joinfs.o joinfs.c

obj/jfs_file_cache.o:
	$(CC) -c $(CFLAGS) obj/jfs_file_cache.o jfs_file_cache.c

obj/jfs_file.o:
	$(CC) -c $(FUSE) $(CFLAGS) obj/jfs_file.o jfs_file.c

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
	rm -rf obj/*.o $(TESTS) ../demo/joinfs
