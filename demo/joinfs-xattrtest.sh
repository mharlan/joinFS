setfattr -n attr1 -v hello -h mount/test.txt
setfattr -n attr2 -v world -h mount/test.txt
setfattr -n attr3 -v balls -h mount/test.txt
getfattr -n attr1 mount/test.txt
getfattr -n attr2 mount/test.txt
getfattr -n attr3 mount/test.txt
