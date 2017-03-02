https://www.cs.princeton.edu/courses/archive/fall04/cos318/
https://web.cs.wpi.edu/~rek/DCS/D04/UnixFileSystems.html
# File System
Each file has an inode which is a data structure representing the files metadata, each inode has an inode number which is used to address the file.

inode table stored on disk and loaded into kernel memory when disk attached. Also each process has an open file table.

## Super block
https://www.quora.com/Where-are-inodes-stored-in-a-file-system
https://www.cs.princeton.edu/courses/archive/fall04/cos318/precepts/6.pdf
Contains layout of the Disk. Size of Disk, Number of Inodes, Number of Data Blocks, Where inodes start, where data blocks start ...

## Inode structure
total number of inodes is created when a file system is set up.

### Directory
Inode structure of a directory just consists of Name to Inode mapping of files and directories in that directory.

So you can clearly note that inode of .(dot) inside /var/log directory is equal to inode of log directory. And inode of ..(dot dot ) inside /var/log/ is equal to inode of .(dot) inside /var/ directory.
### File
- Mode: This keeps information about two things, one is the permission information, the other is the type of inode, for example an inode can be of a file, directory or a block device etc.
- Owner Info: Access details like owner of the file, group of the file etc.
- Size:  This location store the size of the file in terms of bytes.
- Time Stamps:  it stores the inode creation time, modification time, etc.

Direct Block Pointers
Indirect Block Pointers
Double indirect Block Pointers
Triple Indirect Block Pointers



## Find a file
Whenever a user or a program needs access to a file, the operating system first searches for the exact and unique inode (inode number), in a table called as an inode table. In fact the program or the user who needs access to a file, reaches the file with the help of the inode number found from the inode table.

To reach a particular file with its "name" needs an inode number corresponding to that file. But to reach an inode number you dont require the file name. Infact with the inode number you can get the data.

### Simplifications
- No permissions
- Don't care about locality of data blocks (fragmentation)
- 6  Direct pointers rather then indirect/Double and triple indirect

### TODO
- Design Disk Layout
  - Boot Block
  - Super block
  - inodes
  - data blocks
- File system needs to keep track of all free data blocks, can use a bitmap for this
- implement inode strucutre (only use 6 direct block pointers)
- directory is a type of inode which stores a list of strings pointing to ther inodes
- When a file is opened a file descriptor is created, it is an integer which is used in read/write/seak operations. The file system maintains an entry storing the access mode ot the file, inode number, read pointers within the file...
