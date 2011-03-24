mkdir mount/music_files

cp music/*.MP3 mount/music_files/

mkdir mount/Library
mkdir mount/Library/Music
mkdir mount/Library/Music/.jfs_sub_query
mkdir mount/Library/Music/.jfs_sub_query/.jfs_sub_query

setfattr -n type   -v "music"             mount/music_files/*.MP3
setfattr -n format -v "mp3"               mount/music_files/*.MP3
setfattr -n artist -v "Radiohead"         mount/music_files/*.MP3
setfattr -n album  -v "The King of Limbs" mount/music_files/*.MP3

setfattr -n _jfs_dir_is_dynamic -v "y"                                       mount/Library/Music
setfattr -n _jfs_dir_is_folder  -v "y"                                       mount/Library/Music
setfattr -n _jfs_dir_path_items -v "0"                                       mount/Library/Music
setfattr -n _jfs_dir_key_pairs  -v "k=type;v=music;k=format;v=mp3;k=artist;" mount/Library/Music

setfattr -n _jfs_dir_is_dynamic -v "y"        mount/Library/Music/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "y"        mount/Library/Music/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "1"        mount/Library/Music/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v "k=album;" mount/Library/Music/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y" mount/Library/Music/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "n" mount/Library/Music/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "2" mount/Library/Music/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v ";" mount/Library/Music/.jfs_sub_query/.jfs_sub_query

# sqlite3 joinfs.db 'SELECT f.filename, k.keytext, m.keyvalue FROM files AS f, keys AS k, metadata AS m WHERE k.keyid=m.keyid and f.inode=m.inode;'
