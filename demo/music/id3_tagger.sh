#!/bin/bash

SAVEIFS=$IFS
IFS=$(echo -en "\n\b")

for file in $*
do
    # echo "---Adding metadata for file ${file}"
    
    setfattr -n "type" -v music -h $file
    setfattr -n format -v mp3 -h $file

    val=$(id3 -l -R "${file}" | grep Title | sed "s/Title: //")
    val=$(echo "${val}" | sed 's/ *$//g')
    # echo "---Title:${val}"
    setfattr -n title -v $val -h $file

    val=$(id3 -l -R "${file}" | grep Artist | sed "s/Artist: //")
    val=$(echo "${val}" | sed 's/ *$//g')
    # echo "---Artist:${val}"
    setfattr -n artist -v $val -h $file

    val=$(id3 -l -R "${file}" | grep Album | sed "s/Album: //")
    val=$(echo "${val}" | sed 's/ *$//g')
    # echo "---Album:${val}"
    setfattr -n album -v $val -h $file

    val=$(id3 -l -R "${file}" | grep Year | sed "s/Year: //")
    val=$(echo "${val}" | sed 's/ *$//g')
    # echo "---Year:${val}"
    setfattr -n year -v $val -h $file

    val=$(id3 -l -R "${file}" | grep Genre | sed "s/Genre: //")
    val=$(echo "${val}" | sed 's/ *$//g')
    val=$(echo "${val}" | sed 's/([0-9]\+)//')
    echo "---Genre:${val}"
    setfattr -n genre -v $val -h $file

    val=$(id3 -l -R "${file}" | grep Track | sed "s/Track: //")
    val=$(echo "${val}" | sed 's/ *$//g')
    # echo "---Track:${val}"
    setfattr -n track -v $val -h $file

    val=$(id3 -l -R "${file}" | grep Comment | sed "s/Comment: //")
    val=$(echo "${val}" | sed 's/ *$//g')
    # echo "---Comment:${val}"
    setfattr -n comment -v $val  -h $file

    # echo "----------------------------------\n"
done

IFS=$SAVEIFS


