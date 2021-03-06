CC=gcc
INCLUDE=-I include/
CFLAGS=-g -D_FILE_OFFSET_BITS=64 -Wall -O3

FUSELIB=`pkg-config fuse --cflags --libs` -lfuse
LIBS=-lpthread -lulockmgr -lrt -lsqlite3 $(FUSELIB)

TARGET=../demo

SRC=error_log.c \
    sqlitedb.c \
    thr_pool.c \
    result.c \
    jfs_list.c \
    jfs_file.c \
    joinfs.c \
    jfs_meta.c \
    jfs_util.c \
    jfs_security.c \
    jfs_dir.c \
	jfs_dir_query.c \
	jfs_dynamic_paths.c \
	jfs_datapath_cache.c \
    jfs_key_cache.c \
    jfs_meta_cache.c

OBJS=$(SRC:%.c=obj/%.o)

TESTSRC=tests/jfs_pool_test.c \
		tests/jfs_uuid_test.c \
		tests/jfs_realpath_test.c \
		tests/jfs_meta_test.c \
		tests/jfs_query_builder_test.c

TESTOBJS=obj/error_log.o \
	 	 obj/sqlitedb.o \
	 	 obj/thr_pool.o \
	 	 obj/result.o \
	 	 obj/jfs_list.o \
	 	 obj/jfs_uuid.o

TESTS=$(TESTSRC:%.c=%)

all: $(OBJS)
	$(CC) -ggdb $(CFLAGS) $(LIBS) $(OBJS) -o $(TARGET)/joinfs

nullFS: ../demo/nullFS

../demo/nullFS:
	$(CC) $(CFLAGS) $(FUSELIB) -lulockmgr nullFS/nullFS.c -o ../demo/nullFS

tests: $(OBJS) $(TESTS)

tests/%: tests/%.c
	$(CC) -ggdb $(CFLAGS) $(INCLUDE) $(LIBS) $(TESTOBJS) tests/$*.c -o tests/$*

obj/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $*.c -o obj/$*.o

clean:
	rm -rf obj/*.o $(TESTS) ../demo/joinfs ../demo/nullFS
