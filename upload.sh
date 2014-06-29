#! /bin/bash
sudo mount /dev/sdb1 /mnt
file=/mnt/ED64/OS64.v64
if [ -e $file ]
then
	echo -e "File $file exists - mount ok"
	echo -e "copy..."
	sudo cp OS64.v64 /mnt/ED64/
	sudo cp ALT64.INI /mnt/ED64/
	echo -e "umounting..."
	sudo umount /mnt
	echo -e "done..."
else
	echo -e "File $file doesnt exists - sdcard problem?"
fi
