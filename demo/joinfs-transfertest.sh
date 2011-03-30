./joinfs-stop.sh
./joinfs-reset.sh
./joinfs-start.sh
rm transfer.log

echo "Transfer Test 7200 rpm, Start" >> transfer.log

echo "-------------------"
echo "joinFS Tests"
echo "-------------------" >> transfer.log
echo "joinFS Tests" >> transfer.log

echo "Linux transfer (40,000 files) ~400MiB"
echo "Linux transfer (40,000 files) ~400MiB" >> transfer.log
( time cp -r linux-2.6.38.2/ mount/ ) > transfer.log 2>&1

echo "Music transfer (200 files) ~1.4Gib"
echo "Music transfer (200 files) ~1.4Gib" >> transfer.log
( time cp -r music/ mount/ ) > transfer.log 2>&1

echo "Movie transfer (1 files) ~1.4GiB"
echo "Movie transfer (1 files) ~1.4GiB" >> transfer.log
( time cp -r movies/ mount/ ) > transfer.log 2>&1

./joinfs-stop.sh
./joinfs-reset.sh
./nullFS mount/

rm -r mount/home/matt/test/*

echo "-------------------"
echo "nullFS Tests"
echo "-------------------" >> transfer.log
echo "nullFS Tests" >> transfer.log

echo "Linux transfer (40,000 files) ~400MiB"
echo "Linux transfer (40,000 files) ~400MiB" >> transfer.log
( time cp -r linux-2.6.38.2/ mount/home/matt/test ) > transfer.log 2>&1

echo "Music transfer (200 files) ~1.4Gib"
echo "Music transfer (200 files) ~1.4Gib" >> transfer.log
( time cp -r music/ mount/home/matt/test ) > transfer.log 2>&1

echo "Movie transfer (1 files) ~1.4GiB"
echo "Movie transfer (1 files) ~1.4GiB" >> transfer.log
( time cp -r movies/ mount/home/matt/test ) > transfer.log 2>&1

./joinfs-stop.sh
echo "-------------------"
echo "ext3 Tests"
echo "-------------------" >> transfer.log
echo "ext3 Tests" >> transfer.log

rm -r mount/*

echo "Linux transfer (40,000 files) ~400MiB"
echo "Linux transfer (40,000 files) ~400MiB" >> transfer.log
( time cp -r linux-2.6.38.2/ mount/ ) > transfer.log 2>&1

echo "Music transfer (200 files) ~1.4Gib"
echo "Music transfer (200 files) ~1.4Gib" >> transfer.log
( time cp -r music/ mount/ ) > transfer.log 2>&1

echo "Movie transfer (1 files) ~1.4GiB"
echo "Movie transfer (1 files) ~1.4GiB" >> transfer.log
( time cp -r movies/ mount/ ) > transfer.log 2>&1

