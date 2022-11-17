// Continus File System

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
 
// Constraints on the file System --------------------------------------------------------------------
 
#define TOTAL_SIZE_OF_DISK 40960000
#define TOTAL_AVAILABLE_SIZE_OF_DISK 40902656 // Because of metadata
#define BLOCK_SIZE_LIMIT 4096
#define FILE_NAME_LIMIT 50
#define MAX_DATA_BLOCKS_PER_FILE 100
#define MAX_NUM_OF_DATA_BLOCKS 9986
#define MAX_NUM_OF_FILES 99
#define MAGIC_NUMBER 212223
 
// Data Type declarations -----------------------------------------------------------------------------
 
struct superBlock // 20 + x Bytes
{
   int magicNumber;
   int totalAvailableSize;
   int blockSize;
   int free[MAX_NUM_OF_DATA_BLOCKS]; // Bitmap vector
   int numEmptyBlocks;
   int numFullBlocks;
   // It is a single directory structure so it contains no directory, only files
};
 
struct dataBlock // 4096 Bytes
{
   char data[BLOCK_SIZE_LIMIT];
};
 
struct file // 458 Bytes
{
   int isCreated;
   char fileName[FILE_NAME_LIMIT];
   int dataBlocks[MAX_DATA_BLOCKS_PER_FILE];
   int numBlocks;
};
 
struct directory // 45808 Bytes
{
   struct file files[MAX_NUM_OF_FILES]; // This is the file Allocation table
   int maxFileSize;                     // Maximum number of blocks a file is allowed to occupy
   int maxNumFiles;                     // Maximum number of files allowed on the disk
};
 
FILE *disk; // Pointer to 40 MB disk file
struct superBlock sb;
struct directory dir;
 
// Functions ---------------------------------------------------------------------------------------------
 
void format()
{
   disk = fopen("CFS.data", "w+");
   if (!disk)
   {
       printf("Error mounting the disk! Exiting\n");
       exit(1);
   }
   printf("formating the disk\n");
 
   // Initialization for superblock
   sb.magicNumber = MAGIC_NUMBER;
   sb.totalAvailableSize = TOTAL_AVAILABLE_SIZE_OF_DISK; // in Bytes
   sb.blockSize = BLOCK_SIZE_LIMIT;                      // in Bytes
 
   for (int i = 0; i < MAX_NUM_OF_DATA_BLOCKS; i++)
       sb.free[i] = 0;
 
   sb.numEmptyBlocks = MAX_NUM_OF_DATA_BLOCKS;
   sb.numFullBlocks = 0;
 
   // Initialization for directory
   dir.maxFileSize = MAX_DATA_BLOCKS_PER_FILE; // This is 100 blocks or 100*4096 Bytes
   dir.maxNumFiles = MAX_NUM_OF_FILES;
 
   for (int i = 0; i < dir.maxNumFiles; i++)
       dir.files[i].isCreated = 0;
 
   // Writing the fresh super block and the directory block on the disk
   fwrite(&sb, sizeof(struct superBlock), 1, disk);
   fwrite(&dir, sizeof(struct directory), 1, disk);
}
 
void mount()
{
   printf("Format or not?(1 or 0): ");
   int isformat;
   scanf("%d", &isformat);
 
   if (isformat)
       format();
   else
   {
       // opening the disk for read and write in append mode
       disk = fopen("CFS.data", "r+");
       if (!disk)
       {
           printf("Error mounting the disk! Exiting\n");
           exit(1);
       }
 
       fseek(disk, 0, SEEK_SET);
 
       // Reading the superblock and directory block from the disk
       fread(&sb, sizeof(struct superBlock), 1, disk);
       fread(&dir, sizeof(struct directory), 1, disk);
 
       if (sb.magicNumber == MAGIC_NUMBER)
       {
           printf("Mount Successfully....\n");
       }
       else
       {
           printf("There is no existing disk there to mount\n");
           format();
           printf("Succcessfully mounted a new disk\n");
       }
   }
}
 
struct directory opendir() // Here as if there is only one directory opendir() and readdir() will be same
{
   // there is only one directory in file so return direct dir pointer
   return dir;
}
 
void readdir()
{
   if (sb.numFullBlocks == 0)
       printf("Disk empty. No files on the disk!\n");
   else
   {
 
       printf("\n----------------------------CFS.data--------------------------\n\n");
       printf("File No.\t File Name\n");
       for (int i = 0; i < dir.maxNumFiles; i++)
       {
           if (dir.files[i].isCreated)
               printf("%d\t\t\t\t %s\n", i + 1, dir.files[i].fileName);
       }
   }
   printf("\n");
}
 
void create(char *filename)
{
   if (strlen(filename) > FILE_NAME_LIMIT)
   {
       printf("Too long file name\n");
       return;
   }
   // Finding an empty space in file allocation table for the new filer
   int fileIndex = 0;
   for (; fileIndex < dir.maxNumFiles; fileIndex++)
   {
       if (dir.files[fileIndex].isCreated == 0)
           break;
   }
 
   for (int i = 0; i < FILE_NAME_LIMIT; ++i)
       dir.files[fileIndex].fileName[i] = filename[i];
 
   // Moving file pointer to the beginning of the disk
 
   // Updating the file table in directory
   sb.numFullBlocks++;
   dir.files[fileIndex].isCreated = 1;
 
   // update disk meta data
   fseek(disk, 0, SEEK_SET);
   fwrite(&sb, sizeof(struct superBlock), 1, disk);
   fwrite(&dir, sizeof(struct directory), 1, disk);
}
 
void unlink(char *filename)
{
   if (strlen(filename) > FILE_NAME_LIMIT)
   {
       printf("Too long file name\n");
       return;
   }
   // Looking for the required file in the directory
   int fileIndex = 0;
   for (fileIndex = 0; fileIndex < dir.maxNumFiles; fileIndex++)
   {
       if (strcmp(dir.files[fileIndex].fileName, filename) == 0)
           break;
   }
 
   if (fileIndex == dir.maxNumFiles)
       printf("No such file present on the disk!\n");
 
   else
   {
       for (int i = 0; i < 10; i++)
       {
           if (dir.files[fileIndex].dataBlocks[i] != -1)
               sb.free[dir.files[fileIndex].dataBlocks[i]] = 0;
           else
               break;
       }
       dir.files[fileIndex].isCreated = 0;
       sb.numFullBlocks--;
       printf("\nFile named %s deleted\n", dir.files[fileIndex].fileName);
 
       fseek(disk, 0, SEEK_SET);
 
       // update disk meta data
       fwrite(&sb, sizeof(struct superBlock), 1, disk);
       fwrite(&dir, sizeof(struct directory), 1, disk);
   }
}
 
void unmount()
{
   if (disk != NULL)
   {
       fclose(disk);
       printf("\nUnmount successful!\n");
   }
}
 
int main()
{
   // Mounting the disk
   mount();
 
   struct directory currentDirectory = opendir(); // Opening current directory
 
   readdir(); // Listing files in current directory
   create("File1.txt");
   create("File2.txt");
 
   readdir();
   unlink("File2.txt");
 
   readdir();
   // Unmounting the disk
   unmount();
   return 0;
}
