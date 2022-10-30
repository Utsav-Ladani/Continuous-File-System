// Continus File System

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Constraints on the file System --------------------------------------------------------------------

#define TOTAL_SIZE_OF_DISK 40960000
#define BLOCK_SIZE_LIMIT 4096
#define FILE_NAME_LIMIT 50
#define MAX_DATA_BLOCKS_PER_FILE 100
#define MAX_NUM_OF_DATA_BLOCKS 10000
#define MAX_NUM_OF_FILES 100

// Data Type declarations -----------------------------------------------------------------------------

struct file
{
	int isCreated;
	char fileName[FILE_NAME_LIMIT];
	int dataBlocks[MAX_DATA_BLOCKS_PER_FILE];
	int numBlocks;
};

struct superBlock
{
	int totalSize;
	int blockSize;
	int totalNumBlocks;
	int free[MAX_NUM_OF_DATA_BLOCKS]; // Bitmap vector
	int numEmptyBlocks;
	int numFullBlocks;
	// It is a single directory structure so it contains no directory, only files
};

struct directory
{
	struct file files[MAX_NUM_OF_FILES]; // This is the file Allocation table
	int maxFileSize;					 // Maximum number of blocks a file is allowed to occupy
	int maxNumFiles;					 // Maximum number of files allowed on the disk
};

struct dataBlock
{
	char data[BLOCK_SIZE_LIMIT];
};

FILE *disk;
struct superBlock sb;
struct directory dir;

// Functions ---------------------------------------------------------------------------------------------

void prompt()
{
	printf("Main Menu:\n\n");
	printf("1. List files in the disk\n");
	printf("2. Create file\n");
	printf("3. Read file\n");
	printf("4. Edit a file\n");
	printf("5. Delete a file\n");
	printf("Any other number to exit\n\n");
	printf("Enter choice: ");
}

void mount()
{

	printf("Mounting Disk...\n");
	printf("Do you want to format the disk or use it as it is?(1 or 2): ");
	int format;
	scanf("%d", &format);

	if (format == 1)
	{
		disk = fopen("CFS.data", "w+");
		if (!disk)
		{
			printf("Error mounting the disk! Exiting\n");
			exit(1);
		}
		else
			printf("Mount successful!\n\n");

		// Initialization for superblock
		sb.totalSize = TOTAL_SIZE_OF_DISK; // in Bytes
		sb.blockSize = BLOCK_SIZE_LIMIT;   // in Bytes
		sb.totalNumBlocks = sb.totalSize / sb.blockSize;

		for (int i = 0; i < sb.totalNumBlocks; i++)
			sb.free[i] = 0;

		sb.numEmptyBlocks = MAX_NUM_OF_DATA_BLOCKS;
		sb.numFullBlocks = 0;

		// Initialization for directory
		dir.maxFileSize = MAX_DATA_BLOCKS_PER_FILE; // This is 10 blocks or 10*4096 Bytes
		dir.maxNumFiles = sb.totalNumBlocks / dir.maxFileSize;

		for (int i = 0; i < dir.maxNumFiles; i++)
			dir.files[i].isCreated = 0;

		// Writing the fresh super block and the directory block on the disk
		fwrite(&sb, sizeof(struct superBlock), 1, disk);
		fwrite(&dir, sizeof(struct directory), 1, disk);
	}
	else
	{
		// opening the disk for read and write in append mode
		disk = fopen("CFS.data", "a+");
		if (!disk)
		{
			printf("Error mounting the disk! Exiting\n");
			exit(1);
		}
		else
			printf("Mount successful!\n\n");

		// Reading the superblock and directory block from the disk
		fread(&sb, sizeof(struct superBlock), 1, disk);
		fread(&dir, sizeof(struct directory), 1, disk);
	}
}

void listFiles()
{
	if (sb.numFullBlocks == 0)
		printf("Disk empty. No files on the disk!\n");
	else
	{
		printf("\n----------------------------CFS.data--------------------------\n\n");
		printf("File No.\t File Name\t\t\t\t\t\t\t\t Number of Blocks Occupied\n");
		for (int i = 0; i < dir.maxNumFiles; i++)
		{
			if (dir.files[i].isCreated)
				printf("%d\t\t\t\t %s\t %d\n", i + 1, dir.files[i].fileName, dir.files[i].numBlocks);
		}
	}
	printf("\n");
}

void createFile()
{

	// Finding an empty space in file allocation table for the new filer
	int fileIndex;
	for (fileIndex = 0; fileIndex < dir.maxNumFiles; fileIndex++)
	{
		if (!dir.files[fileIndex].isCreated)
			break;
	}

	// Getting file name of the new file from the user
	printf("Enter the name of the file to create: ");
	scanf("%s", dir.files[fileIndex].fileName);

	// Initializing the block list for the newly created file
	for (int i = 0; i < MAX_DATA_BLOCKS_PER_FILE; i++)
		dir.files[fileIndex].dataBlocks[i] = -1;

	printf("\n---------Enter data (input # when finished)------------\n\n");

	char inputChar = ' ';
	int numBlocks = 1;
	int blockIndex = 0;

	while (inputChar != '#')
	{

		// Finding a free datablock on disk
		int isFree = 0;
		for (; blockIndex < sb.totalNumBlocks; blockIndex++)
		{
			if (!sb.free[blockIndex])
			{
				isFree = 1;
				break;
			}
		}

		if (isFree == 0)
		{
			printf("All blocks are full.\n");
			return;
		}

		// Moving the file pointer to the appropriate position
		// fseek(disk, (blockIndex-(ftell(disk) - sizeof(struct superBlock) - sizeof(struct directory)))*sb.blockSize, SEEK_CUR);
		fseek(disk, blockIndex * sb.blockSize + sizeof(struct superBlock) + sizeof(struct directory), SEEK_SET);

		// Getting user input for file data
		struct dataBlock db;
		int i;
		for (i = 0; i < sb.blockSize; i++)
		{
			// Getting input from the user
			if ((inputChar = getchar()) != '#')
				db.data[i] = inputChar;
			else
				break;
		}
		if (inputChar == '#')
			db.data[i] = '#';

		// The only data user entered was # so this is an empty file with 0 blocks allocated to it
		if (numBlocks == 1 && strcmp(db.data, "#") == 0)
			dir.files[fileIndex].numBlocks = 0;

		// For non empty files
		else
		{

			if (numBlocks > MAX_DATA_BLOCKS_PER_FILE)
			{
				printf("Input is too big. %d\n", numBlocks);
				return;
			}
			// Writing the user input to the file
			fwrite(&db, sizeof(struct dataBlock), 1, disk);

			// Updating the bit map in the super block
			sb.free[blockIndex] = 1;

			// Adding the location of this datablock to the list of data blocks for this file
			dir.files[fileIndex].dataBlocks[numBlocks - 1] = blockIndex;
			blockIndex++;

			if (inputChar != '#')
				numBlocks++;
		}
	}

	// Updating the number of blocks this file takes up in the directory block
	dir.files[fileIndex].numBlocks = numBlocks;

	// Moving file pointer to the beginning of the disk
	fseek(disk, 0, SEEK_SET);

	// Updating the number of Full and Empty datablocks in the superblock
	sb.numEmptyBlocks -= dir.files[fileIndex].numBlocks;
	sb.numFullBlocks += dir.files[fileIndex].numBlocks;

	// Writing the updates superblock back to the disk
	fwrite(&sb, sizeof(struct superBlock), 1, disk);

	// Updating the file table in directory
	dir.files[fileIndex].isCreated = 1;

	// Writing the updated directory block back to the disk
	fwrite(&dir, sizeof(struct directory), 1, disk);
}

void readFile()
{

	// Getting filename of the file that the user wants to read
	printf("Enter the name of the file to read: ");

	char inputName[100];
	scanf("%s", inputName);

	// Looking for the required file in the directory
	int fileIndex;
	for (fileIndex = 0; fileIndex < dir.maxNumFiles; fileIndex++)
	{
		if (strcmp(dir.files[fileIndex].fileName, inputName) == 0)
			break;
	}

	if (fileIndex == dir.maxNumFiles)
		printf("No such file present on the disk!\n");

	else
	{
		printf("\n------------------------------%s----------------------------\n\n", dir.files[fileIndex].fileName);
		for (int i = 0; i < MAX_DATA_BLOCKS_PER_FILE; i++)
		{

			// If this is the end of the data block list for this file then nothing left to read, come out of the loop
			if (dir.files[fileIndex].dataBlocks[i] == -1)
				break;

			// else
			//  Move the file pointer to this data block
			//  fseek(disk, (dir.files[fileIndex].dataBlocks[i]-(ftell(disk)- sizeof(struct superBlock) - sizeof(struct directory)))*sb.blockSize, SEEK_CUR);
			fseek(disk, dir.files[fileIndex].dataBlocks[i] * sb.blockSize + sizeof(struct superBlock) + sizeof(struct directory), SEEK_SET);

			// Read the data in the data block
			struct dataBlock db;
			fread(&db, sizeof(struct dataBlock), 1, disk);

			// Print the read data character by character
			for (int i = 0; i < sb.blockSize && db.data[i] != '#'; i++)
				printf("%c", db.data[i]);
		}
		printf("\n");
	}
}

void editFile()
{

	// Getting filename of the file that the user wants to read
	printf("Enter the name of the file to edit: ");

	char inputName[100];
	scanf("%s", inputName);

	// Looking for the required file in the directory
	int fileIndex;
	for (fileIndex = 0; fileIndex < dir.maxNumFiles; fileIndex++)
	{
		if (strcmp(dir.files[fileIndex].fileName, inputName) == 0)
			break;
	}

	if (fileIndex == dir.maxNumFiles)
		printf("No such file present on the disk!\n");

	else
	{
		printf("\n\n1. Edit the block\n-. Any other value to Remove the block\n\n-------------------------------------------------------\n\n");
		int i;
		for (i = 0; i < 10; i++)
		{
			if (dir.files[fileIndex].dataBlocks[i] == -1)
				break;

			// Moving the file pointer to this data block
			fseek(disk, dir.files[fileIndex].dataBlocks[i] * sb.blockSize + sizeof(struct superBlock) + sizeof(struct directory), SEEK_SET);

			// Reading the data in the data block
			struct dataBlock db;
			fread(&db, sizeof(struct dataBlock), 1, disk);

			// Print the read data character by character
			for (int i = 0; i < sb.blockSize && db.data[i] != '#'; i++)
				printf("%c", db.data[i]);
			printf("\n\n");

			printf("Enter your choice: ");
			int input;
			scanf("%d", &input);

			// Edit the block
			if (input == 1)
			{
				printf("\nEnter the new data for this block (# to stop taking the input for this block)\n");
				char removeSpace;
				scanf("%c", &removeSpace);
				fseek(disk, dir.files[fileIndex].dataBlocks[i] * sb.blockSize + sizeof(struct superBlock) + sizeof(struct directory), SEEK_SET);
				char inputChar;
				struct dataBlock db;
				int j;
				for (j = 0; j < sb.blockSize; j++)
				{
					if ((inputChar = getchar()) == '#' || inputChar == '*')
						break;
					db.data[j] = inputChar;
				}

				// The user left the block empty so there is not data for this block, we can mark it as free
				if (strcmp(db.data, "#") == 0)
				{
					sb.free[dir.files[fileIndex].dataBlocks[i]] = 0;
					for (int j = i; dir.files[fileIndex].dataBlocks[j] != -1; j++)
						dir.files[fileIndex].dataBlocks[j] = dir.files[fileIndex].dataBlocks[j + 1];
					dir.files[fileIndex].numBlocks--;
					sb.numFullBlocks--;
					sb.numEmptyBlocks++;
				}
				// Otherwise write the new data for this block
				else
				{
					if (inputChar == '#')
						db.data[j] = '#';
					fwrite(&db, sizeof(struct dataBlock), 1, disk);
				}
			}

			// Deleting the block
			else
			{
				// Marking this block as free in the bit map
				sb.free[dir.files[fileIndex].dataBlocks[i]] = 0;
				// Removing this block from the file's data block list
				for (int j = i; dir.files[fileIndex].dataBlocks[j] != -1; j++)
					dir.files[fileIndex].dataBlocks[j] = dir.files[fileIndex].dataBlocks[j + 1];
				// Decrementing the number of blocks for this file
				dir.files[fileIndex].numBlocks--;
				// Updating the number of Empty and Full blocks in the superBlock
				sb.numFullBlocks--;
				sb.numEmptyBlocks++;
			}
		}
		printf("\n");

		// Moving the file pointer back to the start of the first data block
		fseek(disk, sizeof(struct superBlock) + sizeof(struct directory), SEEK_SET);
	}
}

void deleteFile()
{

	// Getting filename of the file that the user wants to delete
	printf("Enter the name of the file to delete: ");
	char removeSpace;
	scanf("%c", &removeSpace);

	char inputName[100];
	scanf("%s", inputName);

	// Looking for the required file in the directory
	int fileIndex;
	for (fileIndex = 0; fileIndex < dir.maxNumFiles; fileIndex++)
	{
		if (strcmp(dir.files[fileIndex].fileName, inputName) == 0)
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
		sb.numEmptyBlocks += dir.files[fileIndex].numBlocks;
		sb.numFullBlocks -= dir.files[fileIndex].numBlocks;
		printf("\nFile named %s deleted\n", dir.files[fileIndex].fileName);
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

	char retToMainMenu;

	do
	{
		// Printing the main menu
		prompt();
		int choice;
		scanf("%d", &choice);

		if (choice == 1)
			listFiles();
		else if (choice == 2)
			createFile();
		else if (choice == 3)
			readFile();
		else if (choice == 4)
			editFile();
		else if (choice == 5)
			deleteFile();
		else
			break;
		printf("Do you want to return to main menu?(y/n): ");
		scanf(" %c", &retToMainMenu);

	} while (retToMainMenu == 'y');

	// Unmounting the disk
	unmount();
	return 0;
}
