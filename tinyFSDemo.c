#include <stdio.h>
#include "libTinyFS.h"
#include <time.h>

int main(void) {

	int file1, file2, file3;
	char buffer[BLOCKSIZE] = {"Andrew and Barbara"};
	char read;
	int test;
	time_t file1_creation;

	//printf("TFS Make FS %d\n",tfs_mkfs("tinyFSDisk",DEFAULT_DISK_SIZE));

	printf("TFS Mount %d\n", tfs_mount("tinyFSDisk"));
	printf("TFS Open File 1: %d\n",file1 = tfs_openFile("test1"));
	
	file1_creation = tfs_readFileInfo(file1);
	printf("The time is %ld %s\n", file1_creation, ctime(&file1_creation));

	printf("TFS Open File 1.5: %d\n",file1 = tfs_openFile("test1"));
	
	printf("TFS Open File 2: %d\n", file2 = tfs_openFile("test2"));
		
	tfs_displayFragments();
	printf("TFS Write File 1: %d\n",tfs_writeFile(file1, buffer, 300));
	tfs_displayFragments();
	printf("TFS Write File 2: %d\n",tfs_writeFile(file2, buffer, sizeof(buffer)));
	tfs_displayFragments();

	printf("TFS Write File 2: %d\n",tfs_writeByte(file2, 89));

	printf("TFS Read Files and Directories: %d\n",tfs_readdir());

	printf("TFS Read Files and Directories: %d\n",tfs_readdir());
	printf("TFS Read Byte File 2: %d\n",tfs_readByte(file2,&read));
	printf("TFS Read Byte File 2: %d\n",tfs_readByte(file2,&read));
	printf("TFS Read Byte File 2: %d\n",tfs_readByte(file2,&read));
	printf("TFS Read Byte File 2: %d\n",tfs_readByte(file2,&read));

	printf("TFS Seek File 2: %d\n",tfs_seek(file2,11));
	printf("TFS Read Byte File 2: %d\n",tfs_readByte(file2,&read));
	printf("TFS Rename File 2: %d\n",tfs_rename(file2, "test3"));

	file3 = file2;
	printf("TFS Make file1 Read-Only: %d\n",tfs_makeRO("test3"));
	printf("TFS Write Byte to file 1: %d\n",tfs_writeByte(file2,89));
	
	printf("TFS Delete File 1: %d\n",tfs_deleteFile(file1));
/*
	printf("TFS Delete File 2: %d\n",tfs_deleteFile(file2));

	printf("TFS Read Files and Directories: %d\n",tfs_readdir());

	printf("TFS Make file1 Read-Write: %d\n",tfs_makeRW("test3"));
	printf("TFS Write Byte to file 1: %d\n",tfs_writeByte(file3,89));
		
	printf("TFS Delete File 2: %d\n",tfs_deleteFile(file2));

	printf("TFS Read Files and Directories: %d\n",tfs_readdir());
*/
	tfs_displayFragments();
	tfs_defrag();
	tfs_displayFragments();

	return 0;
} 
