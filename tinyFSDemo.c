#include <stdio.h>
#include "libTinyFS.h"
#include <time.h>

int main(void) {

	int file1, file2, file3;
	char buffer1[BLOCKSIZE] = {"Andrew"};
	char buffer2[BLOCKSIZE] = {"Barbara"};
        char buffer3[BLOCKSIZE] = {"Ryan"};
        char read;
	int test;
	time_t file1_creation;

	printf("TFS Make FS %d\n",tfs_mkfs("tinyFSDisk",DEFAULT_DISK_SIZE));

	printf("TFS Mount %d\n", tfs_mount("tinyFSDisk"));
	printf("TFS Open File 1: %d\n",file1 = tfs_openFile("test1"));
	tfs_displayFragments();
	
	file1_creation = tfs_readFileInfo(file1);
	printf("The time is %ld %s\n", file1_creation, ctime(&file1_creation));

	printf("TFS Open File 2: %d\n",file2 = tfs_openFile("test2"));
	tfs_displayFragments();
	
	printf("TFS Open File 3: %d\n", file3 = tfs_openFile("test3"));
	tfs_displayFragments();
		
	printf("TFS Write File 1: %d\n",tfs_writeFile(file1, buffer1, 256));
	tfs_displayFragments();
	printf("TFS Write File 2: %d\n",tfs_writeFile(file2, buffer2, 512));
	tfs_displayFragments();

	printf("TFS Write File 2: %d\n",tfs_writeByte(file2, 89));
	tfs_displayFragments();

	printf("TFS Read Files and Directories: %d\n",tfs_readdir());

	printf("TFS Write File 3: %d\n",tfs_writeFile(file3, buffer3, 1024));
	tfs_displayFragments();
	
	printf("TFS Delete File 1: %d\n",tfs_deleteFile(file1));
	tfs_displayFragments();
/*
	printf("TFS Delete File 1: %d\n",tfs_deleteFile(file1));

	printf("TFS Read Files and Directories: %d\n",tfs_readdir());

	printf("TFS Make file1 Read-Write: %d\n",tfs_makeRW("test3"));
	printf("TFS Write Byte to file 1: %d\n",tfs_writeByte(file3,89));
		
	printf("TFS Delete File 2: %d\n",tfs_deleteFile(file2));

	printf("TFS Read Files and Directories: %d\n",tfs_readdir());
*/
        printf("DEFRAG\n");
	tfs_defrag();
	tfs_displayFragments();

        printf("TFS Delete File 2: %d\n",tfs_deleteFile(file2));
        tfs_displayFragments();
        printf("DEFRAG\n");
        tfs_defrag();
        tfs_displayFragments();
	return 0;
} 
