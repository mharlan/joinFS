./joinfs-stop.sh
./joinfs-reset.sh
./joinfs-start.sh

echo "Transfer Test 7200 rpm, Start" >> transfer.log

echo "-------------------"
echo "joinFS Tests"
echo "-------------------" >> transfer.log
echo "joinFS Tests" >> transfer.log

echo "Linux transfer (40,000 files) ~400MiB"
echo "Linux transfer (40,000 files) ~400MiB" >> transfer.log
date >> transfer.log
cp -r linux-2.6.38.2/ mount/
date >> transfer.log

echo "Music transfer (200 files) ~1.4Gib"
echo "Music transfer (200 files) ~1.4Gib" >> transfer.log
date >> transfer.log
cp -r music/ mount/
date >> transfer.log

echo "Movie transfer (1 files) ~1.4GiB"
echo "Movie transfer (1 files) ~1.4GiB" >> transfer.log
date >> transfer.log
cp -r movies/ mount/
date >> transfer.log

./joinfs-stop.sh
./joinfs-reset.sh
./nullFS mount/

echo "-------------------"
echo "nullFS Tests"
echo "-------------------" >> transfer.log
echo "nullFS Tests" >> transfer.log
rm -r mount/home/matt/test/*

echo "Linux transfer (40,000 files) ~400MiB"
echo "Linux transfer (40,000 files) ~400MiB" >> transfer.log
date >> transfer.log
cp -r linux-2.6.38.2/ mount/home/matt/test
date >> transfer.log

echo "Music transfer (200 files) ~1.4Gib"
echo "Music transfer (200 files) ~1.4Gib" >> transfer.log
date >> transfer.log
cp -r music/ mount/home/matt/test
date >> transfer.log

echo "Movie transfer (1 files) ~1.4GiB"
echo "Movie transfer (1 files) ~1.4GiB" >> transfer.log
date >> transfer.log
cp -r movies/ mount/home/matt/test
date >> transfer.log

./joinfs-stop.sh
echo "-------------------"
echo "ext3 Tests"
echo "-------------------" >> transfer.log
echo "ext3 Tests" >> transfer.log

rm -r mount/*

echo "Linux transfer (40,000 files) ~400MiB"
echo "Linux transfer (40,000 files) ~400MiB" >> transfer.log
date >> transfer.log
cp -r linux-2.6.38.2/ mount/
date >> transfer.log

echo "Music transfer (200 files) ~1.4Gib"
echo "Music transfer (200 files) ~1.4Gib" >> transfer.log
date >> transfer.log
cp -r music/ mount/
date >> transfer.log

echo "Movie transfer (1 files) ~1.4GiB"
echo "Movie transfer (1 files) ~1.4GiB" >> transfer.log
date >> transfer.log
cp -r movies/ mount/
date >> transfer.log

