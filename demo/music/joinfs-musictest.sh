# set metdata
./id3_tagger.sh *.mp3

mkdir ../Library

# MP3s - Artist/Album/File
mkdir ../Library/Music
mkdir ../Library/Music/.jfs_sub_query
mkdir ../Library/Music/.jfs_sub_query/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y"                                       ../Library/Music
setfattr -n _jfs_dir_is_folder  -v "y"                                       ../Library/Music
setfattr -n _jfs_dir_path_items -v "0"                                       ../Library/Music
setfattr -n _jfs_dir_key_pairs  -v "k=type;v=music;k=format;v=mp3;k=artist;" ../Library/Music

setfattr -n _jfs_dir_is_dynamic -v "y"        ../Library/Music/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "y"        ../Library/Music/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "1"        ../Library/Music/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v "k=album;" ../Library/Music/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y" ../Library/Music/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "n" ../Library/Music/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "2" ../Library/Music/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v ";" ../Library/Music/.jfs_sub_query/.jfs_sub_query

# All MP3s
mkdir ../Library/Music_All

setfattr -n _jfs_dir_is_dynamic -v "y"                              ../Library/Music_All
setfattr -n _jfs_dir_is_folder  -v "n"                              ../Library/Music_All
setfattr -n _jfs_dir_path_items -v "0"                              ../Library/Music_All
setfattr -n _jfs_dir_key_pairs  -v "k=type;v=music;k=format;v=mp3;" ../Library/Music_All

# MP3s - Genre/Artist/Files
mkdir ../Library/Music_Genres
mkdir ../Library/Music_Genres/.jfs_sub_query
mkdir ../Library/Music_Genres/.jfs_sub_query/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y"                                       ../Library/Music_Genres
setfattr -n _jfs_dir_is_folder  -v "y"                                       ../Library/Music_Genres
setfattr -n _jfs_dir_path_items -v "0"                                       ../Library/Music_Genres
setfattr -n _jfs_dir_key_pairs  -v "k=type;v=music;k=format;v=mp3;k=genre;"  ../Library/Music_Genres

setfattr -n _jfs_dir_is_dynamic -v "y"         ../Library/Music_Genres/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "y"         ../Library/Music_Genres/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "1"         ../Library/Music_Genres/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v "k=artist;" ../Library/Music_Genres/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y" ../Library/Music_Genres/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "n" ../Library/Music_Genres/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "2" ../Library/Music_Genres/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v ";" ../Library/Music_Genres/.jfs_sub_query/.jfs_sub_query

