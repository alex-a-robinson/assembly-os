# Stage 2
## Part A - Dynamic process creation
- Implemented fork & exec
  - Duplicates current process, updaets ppid
  - Returns 0 to child, and pid of child to parent
  - Exec is run on child to switch program
- Implementes parent processes
- Cleans up zombie processes
  - System adopts them by setting their ppid to 0
- Implemented kill
  - If killing current process, reset ctx and run scheduler
  - Update waiting processes with return value
  - otherwise reset process and continue with current execution
  - Implement SIG TERM signal
## Part B - Scheduling algorithm
- Implement priority based scheduler
- Processes can have interactive or background type
- Interactive tasks are intersperced between background tasks
- Task with the longest waiting time done first in each queue
- interactive tasks have a smaller time slice
- This helps to make system feel more interactive
- No chance of starvation
- Processes started by interactive processes inheriate the interactive type. Unless forked.
- Experimented with counting IO access however no success which is not the case with round robin
## Part C - IPC
- Shared memory model with sempahores
- Process can mark a pointer as shared
- Child processes can access this
- blocking and non blocking lock calls
- blocking and non blocking wait calls
- Dinning Philosophers program
  - Solve using arbitrator model, global arbitorator who holds all forks and hands them out to waiting philosophers
  - Problem = slow, however does solve the problem
# Stage 3
## Part B - File System
- Implemented inode file system with 6 data blocks per inode
- Disk layout:
 -------------------------------------------------------
|            |        |                                 |
| SUPERBLOCK | INODES | DATA BLOCKS                     |
|            |        |                                 |
 -------------------------------------------------------
- Superblock contains bitmap for free inodes & free data blocks as well as block size, etc.
- 1KB block size, 68MB disk, Max file size = 6 * 1KB = 6KB
- Disk Initilisation
  - Detection of valid superblock
    - If not found write a superblock, inodes, and IO devices
  - Read root directory
  - Open IO devices
- Implemented open, close, write, read system calls
  - Handle file descriptors + flags (read/write)
  - Only one process can access a file at a time, unless marked as global read/write (as IO devices are)
- Implemented ls, stat
  - List and get file stats e.g. file type
- Implemented mkdir, rmdir, rm system calls
  - Nested directoires, max 10 files per directory
  - Directories initilised with "." and ".." files which link to parent directory and self
- Implemented mount & unmount
  - Gracefully mount and unmount disk, generic enough to use for other disks
  - mount loads superblock into memory, unmount saves to disk
- Implemented device type inode
  - /dev/stdin, /dev/stdout, /dev/stderr are files which can be read and written too by user
- Implemented uesr programs
  - cat, vim, cd, echo, ls, mkdir, ps, rm, rmdir, stat, wc
  - Implemented shell with CWD enviromental varible
