cp test_data/foldertest1.txt mount/foldertest1.txt
cp test_data/foldertest2.txt mount/foldertest2.txt
cp test_data/foldertest3.txt mount/foldertest3.txt
mkdir mount/dirtest1
mkdir mount/dirtest2
mkdir mount/dirtest2/.jfs_sub_query

setfattr -n test_name -v "foldertest1" mount/foldertest1.txt
setfattr -n test_name -v "foldertest2" mount/foldertest2.txt
setfattr -n test_name -v "foldertest3" mount/foldertest3.txt

setfattr -n _jfs_dir_is_dynamic -v "y"            mount/dirtest1
setfattr -n _jfs_dir_is_folder  -v "n"            mount/dirtest1
setfattr -n _jfs_dir_path_items -v "0"            mount/dirtest1
setfattr -n _jfs_dir_key_pairs  -v "k=test_name;" mount/dirtest1

setfattr -n _jfs_dir_is_dynamic -v "y"            mount/dirtest2
setfattr -n _jfs_dir_is_folder  -v "y"            mount/dirtest2
setfattr -n _jfs_dir_path_items -v "0"            mount/dirtest2
setfattr -n _jfs_dir_key_pairs  -v "k=test_name;" mount/dirtest2

setfattr -n _jfs_dir_is_dynamic -v "y" mount/dirtest2/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "n" mount/dirtest2/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "1" mount/dirtest2/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v ";" mount/dirtest2/.jfs_sub_query

sqlite3 joinfs.db 'SELECT f.filename, k.keytext, m.keyvalue FROM files AS f, keys AS k, metadata AS m WHERE k.keyid=m.keyid and f.inode=m.inode;'