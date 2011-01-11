rm mount/test.mp3
"hello world" > mount/test.mp3
../src/tests/jfs_meta_test
sqlite3 joinfs.db 'SELECT k.keytext, m.keyvalue FROM keys AS k, metadata AS m WHERE k.keyid=m.keyid;'
