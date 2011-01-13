cp test_data/foldertest1.txt mount/foldertest1.txt
cp test_data/foldertest2.txt mount/foldertest2.txt
cp test_data/foldertest3.txt mount/foldertest3.txt
mkdir mount/dirtest1
mkdir mount/dirtest2
mkdir mount/dirtest2/.jfs_sub_query
setfattr -n test_name -v "foldertest1" mount/foldertest1.txt
setfattr -n test_name -v "foldertest2" mount/foldertest2.txt
setfattr -n test_name -v "foldertest3" mount/foldertest3.txt
sqlite3 joinfs.db 'UPDATE OR ROLLBACK directories SET query="SELECT f.inode, f.filename, f.datapath FROM files AS f, metadata AS m, keys As k WHERE f.inode=m.inode and m.keyid=k.keyid and k.keytext=%s;", sub_key="test_name" WHERE dirpath="/dirtest1";'
sqlite3 joinfs.db 'UPDATE OR ROLLBACK directories SET query="SELECT m.keyvalue FROM keys AS k, metadata AS m WHERE m.keyid=k.keyid and k.keytext=%s;", has_subquery=1, sub_key="test_name" WHERE dirpath="/dirtest2";'
sqlite3 joinfs.db 'UPDATE OR ROLLBACK directories SET query="SELECT f.inode, f.filename, f.datapath FROM files AS f, metadata AS m, keys AS k WHERE f.inode=m.inode and m.keyid=k.keyid and k.keytext=%s and m.keyvalue=%s;", uses_filename=1, is_subquery=1, sub_key="test_name" WHERE dirpath="/dirtest2/.jfs_sub_query";'
sqlite3 joinfs.db 'SELECT * FROM files;'
sqlite3 joinfs.db 'SELECT * FROM directories;'
sqlite3 joinfs.db 'SELECT * FROM metadata;'
