# ouichefs-log-rotation
Adds log rotation mechanisms to the Ouichefs file system  
The mechanisms are added as kernel modules to the system, two policies are supported:
- Deleting the oldest file.
- Deleting the largest file.

## Purpose
Server software such as web servers typically log every event, request and error to log files, this can rapidly eat up all available space.  
A common approach to managing log files is to have a regularly scheduled **log rotation**, typically a **cron** job that runs the **logrotate** command that renames, deletes or archives the files once they are too old or too big.  
Another approach that is easier and requires less configuration is to use a log rotation capable file system.

## Installation
Download **ouichefs-log-rotation**
```
git clone https://github.com/0x000A/ouichefs-log-rotation.git
```
Download and patch **ouichefs** file system
```
cd ouichefs-log-rotation
git clone https://github.com/rgouicem/ouichefs.git
cp patch.txt ouichefs && cd ouichefs
patch -p1 < patch.txt
```
Build and insert the **ouichefs**
NOTE: If you wish to build the module against a different kernel, run `make KERNELDIR=<path>`
```
make
insmod ouichefs.ko
```
Build `mkfs.ouichefs` and create a **ouichefs** partition
```
cd mkfs && make
```
Create and mount a **ouichefs** partition
```
```

