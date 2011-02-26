CC=gcc
INCLUDE=-I include/
CFLAGS=-D_FILE_OFFSET_BITS=64 -Wall

SQLLIB=-W -O2 -Wl,-R/usr/local/lib -lsqlite3
UUIDLIB=/lib/libuuid.so.1.3.0
FUSELIB=`pkg-config fuse --cflags --libs` -lfuse
LIBS=-lpthread -lrt $(SQLLIB) $(UUIDLIB) $(FUSELIB)

SRC=error_log.c \
    sqlitedb.c \
    thr_pool.c \
    result.c \
    jfs_list.c \
    jfs_file.c \
    jfs_file_cache.c \
    joinfs.c \
    jfs_uuid.c \
    jfs_meta.c \
    jfs_util.c \
    jfs_security.c \
    jfs_dir.c \
	jfs_dynamic_paths.c \
	jfs_datapath_cache.c \
    jfs_key_cache.c \
    jfs_meta_cache.c

OBJS=$(SRC:%.c=obj/%.o)

TESTSRC=tests/jfs_pool_test.c \
		tests/jfs_uuid_test.c \
		tests/jfs_realpath_test.c \
		tests/jfs_meta_test.c

TESTOBJS=obj/error_log.o \
	 	 obj/sqlitedb.o \
	 	 obj/thr_pool.o \
	 	 obj/result.o \
	 	 obj/jfs_list.o \
	 	 obj/jfs_uuid.o

TESTS=$(TESTSRC:%.c=%)

all: $(OBJS) $(TESTS)
	$(CC) -ggdb -O0 $(CFLAGS) $(LIBS) $(OBJS) -o ../demo/joinfs

tests/%: tests/%.c
	$(CC) -ggdb -O0 $(CFLAGS) $(INCLUDE) $(LIBS) $(TESTOBJS) tests/$*.c -o tests/$*

obj/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $*.c -o obj/$*.o

clean:
	rm -rf obj/*.o $(TESTS) ../demo/joinfs
