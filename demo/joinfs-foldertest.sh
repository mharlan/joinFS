cp test_data/foldertest1.txt mount/foldertest1.txt
cp test_data/foldertest2.txt mount/foldertest2.txt
cp test_data/foldertest3.txt mount/foldertest3.txt
mkdir mount/dirtest1
mkdir mount/dirtest2
mkdir mount/dirtest2/.jfs_sub_query
setfattr -n test_name -v "foldertest1" mount/foldertest1.txt
setfattr -n test_name -v "foldertest2" mount/foldertest2.txt
setfattr -n test_name -v "foldertest3" mount/foldertest3.txt

setfattr -n _jfs_dir_query -v "SELECT f.inode, f.filename, f.datapath FROM files AS f, metadata AS m, keys As k WHERE f.inode=m.inode and m.keyid=k.keyid and k.keytext=%s;" mount/dirtest1
setfattr -n _jfs_dir_sub_key -v "test_name" mount/dirtest1
setfattr -n _jfs_dir_sub_inode -v "0" mount/dirtest1
setfattr -n _jfs_dir_is_dynamic -v "Y" mount/dirtest1
setfattr -n _jfs_dir_has_subquery -v "N" mount/dirtest1
setfattr -n _jfs_dir_is_subquery -v "N" mount/dirtest1
setfattr -n _jfs_dir_path_items -v "0" mount/dirtest1

setfattr -n _jfs_dir_query -v "SELECT m.keyvalue FROM keys AS k, metadata AS m WHERE m.keyid=k.keyid and k.keytext=%s;" mount/dirtest2
setfattr -n _jfs_dir_sub_key -v "test_name" mount/dirtest2
setfattr -n _jfs_dir_sub_inode -v "0" mount/dirtest2
setfattr -n _jfs_dir_is_dynamic -v "Y" mount/dirtest2
setfattr -n _jfs_dir_has_subquery -v "Y" mount/dirtest2
setfattr -n _jfs_dir_is_subquery -v "N" mount/dirtest2
setfattr -n _jfs_dir_path_items -v "0" mount/dirtest2

setfattr -n _jfs_dir_query -v "SELECT f.inode, f.filename, f.datapath FROM files AS f, metadata AS m, keys AS k WHERE f.inode=m.inode and m.keyid=k.keyid and k.keytext=%s and m.keyvalue=%s;" mount/dirtest2/.jfs_sub_query
setfattr -n _jfs_dir_sub_key -v "test_name" mount/dirtest2/.jfs_sub_query
setfattr -n _jfs_dir_sub_inode -v "0" mount/dirtest2/.jfs_sub_query
setfattr -n _jfs_dir_is_dynamic -v "Y" mount/dirtest2/.jfs_sub_query
setfattr -n _jfs_dir_has_subquery -v "N" mount/dirtest2/.jfs_sub_query
setfattr -n _jfs_dir_is_subquery -v "Y" mount/dirtest2/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "1" mount/dirtest2/.jfs_sub_query

sqlite3 joinfs.db 'SELECT * FROM files;'
sqlite3 joinfs.db 'SELECT * FROM metadata;'
sqlite3 joinfs.db 'SELECT * FROM keys;'