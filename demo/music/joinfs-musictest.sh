# set metdata
./id3_tagger.sh *.mp3

# MP3s - Artist/Album/Title/File
mkdir ../Library
mkdir ../Library/Music4
mkdir ../Library/Music4/.jfs_sub_query
mkdir ../Library/Music4/.jfs_sub_query/.jfs_sub_query
mkdir ../Library/Music4/.jfs_sub_query/.jfs_sub_query/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y"                                       ../Library/Music4
setfattr -n _jfs_dir_is_folder  -v "y"                                       ../Library/Music4
setfattr -n _jfs_dir_path_items -v "0"                                       ../Library/Music4
setfattr -n _jfs_dir_key_pairs  -v "k=type;v=music;k=format;v=mp3;k=artist;" ../Library/Music4

setfattr -n _jfs_dir_is_dynamic -v "y"        ../Library/Music4/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "y"        ../Library/Music4/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "1"        ../Library/Music4/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v "k=album;" ../Library/Music4/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y"        ../Library/Music4/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "y"        ../Library/Music4/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "2"        ../Library/Music4/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v "k=title;" ../Library/Music4/.jfs_sub_query/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y" ../Library/Music4/.jfs_sub_query/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "n" ../Library/Music4/.jfs_sub_query/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "3" ../Library/Music4/.jfs_sub_query/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v ";" ../Library/Music4/.jfs_sub_query/.jfs_sub_query/.jfs_sub_query

# MP3s - Artist/Album/File
mkdir ../Library/Music3
mkdir ../Library/Music3/.jfs_sub_query
mkdir ../Library/Music3/.jfs_sub_query/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y"                                       ../Library/Music3
setfattr -n _jfs_dir_is_folder  -v "y"                                       ../Library/Music3
setfattr -n _jfs_dir_path_items -v "0"                                       ../Library/Music3
setfattr -n _jfs_dir_key_pairs  -v "k=type;v=music;k=format;v=mp3;k=artist;" ../Library/Music3

setfattr -n _jfs_dir_is_dynamic -v "y"        ../Library/Music3/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "y"        ../Library/Music3/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "1"        ../Library/Music3/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v "k=album;" ../Library/Music3/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y" ../Library/Music3/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "n" ../Library/Music3/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "2" ../Library/Music3/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v ";" ../Library/Music3/.jfs_sub_query/.jfs_sub_query

# MP3s - Artist/File
mkdir ../Library/Music2
mkdir ../Library/Music2/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y"                                       ../Library/Music2
setfattr -n _jfs_dir_is_folder  -v "y"                                       ../Library/Music2
setfattr -n _jfs_dir_path_items -v "0"                                       ../Library/Music2
setfattr -n _jfs_dir_key_pairs  -v "k=type;v=music;k=format;v=mp3;k=artist;" ../Library/Music2

setfattr -n _jfs_dir_is_dynamic -v "y" ../Library/Music2/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "n" ../Library/Music2/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "1" ../Library/Music2/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v ";" ../Library/Music2/.jfs_sub_query

# All MP3s
mkdir ../Library/Music

setfattr -n _jfs_dir_is_dynamic -v "y"                              ../Library/Music
setfattr -n _jfs_dir_is_folder  -v "n"                              ../Library/Music
setfattr -n _jfs_dir_path_items -v "0"                              ../Library/Music
setfattr -n _jfs_dir_key_pairs  -v "k=type;v=music;k=format;v=mp3;" ../Library/Music

mkdir ../Library/Everything4
mkdir ../Library/Everything4/.jfs_sub_query
mkdir ../Library/Everything4/.jfs_sub_query/.jfs_sub_query
mkdir ../Library/Everything4/.jfs_sub_query/.jfs_sub_query/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y"         ../Library/Everything4
setfattr -n _jfs_dir_is_folder  -v "y"         ../Library/Everything4
setfattr -n _jfs_dir_path_items -v "0"         ../Library/Everything4
setfattr -n _jfs_dir_key_pairs  -v "k=level1;" ../Library/Everything4

setfattr -n _jfs_dir_is_dynamic -v "y"         ../Library/Everything4/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "y"         ../Library/Everything4/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "1"         ../Library/Everything4/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v "k=level2;" ../Library/Everything4/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y"         ../Library/Everything4/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "y"         ../Library/Everything4/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "2"         ../Library/Everything4/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v "k=level3;" ../Library/Everything4/.jfs_sub_query/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y" ../Library/Everything4/.jfs_sub_query/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "n" ../Library/Everything4/.jfs_sub_query/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "3" ../Library/Everything4/.jfs_sub_query/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v ";" ../Library/Everything4/.jfs_sub_query/.jfs_sub_query/.jfs_sub_query

mkdir ../Library/Everything3
mkdir ../Library/Everything3/.jfs_sub_query
mkdir ../Library/Everything3/.jfs_sub_query/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y"         ../Library/Everything3
setfattr -n _jfs_dir_is_folder  -v "y"         ../Library/Everything3
setfattr -n _jfs_dir_path_items -v "0"         ../Library/Everything3
setfattr -n _jfs_dir_key_pairs  -v "k=level1;" ../Library/Everything3

setfattr -n _jfs_dir_is_dynamic -v "y"         ../Library/Everything3/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "y"         ../Library/Everything3/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "1"         ../Library/Everything3/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v "k=level2;" ../Library/Everything3/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y" ../Library/Everything3/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "n" ../Library/Everything3/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "2" ../Library/Everything3/.jfs_sub_query/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v ";" ../Library/Everything3/.jfs_sub_query/.jfs_sub_query

mkdir ../Library/Everything2
mkdir ../Library/Everything2/.jfs_sub_query

setfattr -n _jfs_dir_is_dynamic -v "y"         ../Library/Everything2
setfattr -n _jfs_dir_is_folder  -v "y"         ../Library/Everything2
setfattr -n _jfs_dir_path_items -v "0"         ../Library/Everything2
setfattr -n _jfs_dir_key_pairs  -v "k=level1;" ../Library/Everything2

setfattr -n _jfs_dir_is_dynamic -v "y" ../Library/Everything2/.jfs_sub_query
setfattr -n _jfs_dir_is_folder  -v "n" ../Library/Everything2/.jfs_sub_query
setfattr -n _jfs_dir_path_items -v "1" ../Library/Everything2/.jfs_sub_query
setfattr -n _jfs_dir_key_pairs  -v ";" ../Library/Everything2/.jfs_sub_query

mkdir ../Library/Everything

setfattr -n _jfs_dir_is_dynamic -v "y"         ../Library/Everything
setfattr -n _jfs_dir_is_folder  -v "n"         ../Library/Everything
setfattr -n _jfs_dir_path_items -v "0"         ../Library/Everything
setfattr -n _jfs_dir_key_pairs  -v "k=level1;" ../Library/Everything

