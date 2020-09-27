# ouichefs-log-rotation
Adds log rotation mechanisms to the Ouichefs file system  
The mechanisms are added as kernel modules to the system

## Purpose
Server software such as web servers typically log every event, request and error to log files, this can rapidly eat up all available space.  
A common approach to managing log files is to have a regularly scheduled **log rotation** typically a **cron** job that runs the **logrotate** command that renames, deletes or archives the files once they are too old or too big.
