#! /bin/bash

. assert.sh

partition="/test"
tests=20



# Disable dmesg
dmesg -D


# Make sure everything is clean
{
rm -rf $partition/*
umount $partition 
rmmod oldest
rmmod largest
rmmod -f ouichefs
} &> /dev/null


# Mount ouichefs parition
insmod ../ouichefs/ouichefs.ko
mount "../ouichefs/mkfs/test.img" $partition


# Create files for tests
mkdir "$partition/test"

for i in {0..127}
do
	#dd if=/dev/urandom of="file_$i" bs=$2 count=$3
	touch -mt $(date -d @$(shuf -i "0-$(date +%s)" -n 1) +%Y%m%d%H%M.%S) "$partition/test/file_$i"
done


# Store files with the oldest modification dates in an array
oldest_files=($(ls -t "$partition/test" | tail -$tests | tac))



# The touch command increases inode's i_counter so we have to get rid of the
# cache in order to reset all inode's counters to 0
echo 3 > /proc/sys/vm/drop_caches


# Make sure that the directory has 128 file at the start
assert "ls -1 $partition/test | wc -l" "128"
assert_end "count_before_deletion" 


# Start the tests
insmod ../oldest.ko "x=80" 


# Make sure that the right files get deleted each time
i=0
for file in "${oldest_files[@]}"; do
	echo "1" > "/sys/kernel/ouichefs"
	assert "[ -f '/$partition/test/$file' ] || echo 'NOT FOUND'" "NOT FOUND"
	((i+=1))
done

assert_end "remove_oldest_file"



# Clean up everything
{
rmmod oldest 
rm -rf $partition/*
umount $partition 
rmmod -f ouichefs
} &> /dev/null


# Enable dmesg back
dmesg -E
