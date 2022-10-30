# Continuous-File-System

- Continuous File System (CFS) is a simple implementation of Virtal Persistent Continuous File System. 
- CFS support 5 primary functionalities:
  - List files
  - Create/Write files
  - Read files
  - Edit files
  - Delete files
- Language: C
- Platform: Linux

## System Implementation Details

- File system has three different type of blocks.
  - Super Block: Contains metadata about file system
  - Directory: Constains the information about all files
  - Data Block: Contains text data of file.
- Directory has file nodes, which is used to keep record of data blocks of particular file.
- There is also a free space vector to keep record of empty and filled data blocks.
- Implement using basic C functionlities like **read, write, lseek**.
