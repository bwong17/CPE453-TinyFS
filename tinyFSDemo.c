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
	time_t file1_creation, file2_creation;

	printf(">>Start TinyFsDemo program:\n");
	
	printf("tfs_mkfs implementation test: \n\
	 \tCreating new disk 'tinyFSDisk' returns: %d\n",tfs_mkfs("tinyFSDisk",DEFAULT_DISK_SIZE));

	printf("tfs_mmount(tinyFsDisk) returns: %d\n", tfs_mount("tinyFSDisk"));
	printf("Currently mounted disk 'tinyFsDisk' looks like...\n");
	tfs_displayFragments();
	
	printf("tfs_openFile('test1') returns: %d\n",file1 = tfs_openFile("test1"));
	printf("After opening a new file, 'tinyFsDisk' looks like...\n");
	tfs_displayFragments();
	
	printf("But tfs_readdir() just to be sure...\n");
	tfs_readdir();

	file1_creation = tfs_readFileInfo(file1);
	printf("The time 'test1' was created is %s\n", ctime(&file1_creation));


	printf("Let's write some data to 'file1'...\n");
	printf("tfs_writeFile returns %d\n",tfs_writeFile(file1, buffer1, 256));
	printf("After writing to 'test1', 'tinyFsDisk' looks like...\n");
	tfs_displayFragments();
	
	printf("Let's open and write some data to another file after a quick sleep to change the creation date...\n");
	sleep(2);
	printf("tfs_openFile('test2') returns: %d\n",file2 = tfs_openFile("test2"));
	printf("After opening a new file, 'tinyFsDisk' looks like...\n");
	printf("But tfs_readdir() just to be sure...\n");
	tfs_readdir();
	
	printf("tfs_writeFile returns %d\n",tfs_writeFile(file2, buffer2, 256));
	printf("After writing to 'test2', 'tinyFsDisk' looks like...\n");
	tfs_displayFragments();
	
	
	file2_creation = tfs_readFileInfo(file2);
	printf("The time 'test1' was created is %s\n", ctime(&file1_creation));
	printf("The time 'test2' was created is %s\n", ctime(&file2_creation));


	printf("Let's rename test11 to test11\n");
	printf("tfs_rename returns : %d\n", tfs_rename(file1, "test11"));
	
	tfs_readdir();
	tfs_displayFragments();
	
	printf("Let's delete test11...\n");
	printf("tfs_deleteFile(file1) returns: %d\n",tfs_deleteFile(file1));

	tfs_readdir();
	tfs_displayFragments();

	printf("Let's make 'test2' read only and try to delete it...\n");
	printf("tfs_makeRO('test2') returns: %d\n",tfs_makeRO("test2"));
	printf("tfs_deleteFile(file1) returns: %d\n",tfs_deleteFile(file1));

	tfs_readdir();
	tfs_displayFragments();
	
	printf("Good, no changes...let's change it back to RW\n");
	printf("tfs_makeRW('test2') returns: %d\n",tfs_makeRW("test2"));
	
	printf("Let's write a byte ('A') to 'test2'...\n");
	printf("tfs_writeByte to 'test2' returns: %d\n",tfs_writeByte(file2,0x41));
	printf("And now just to make sure let's read the first byte from that file...");
	printf("tfs_readByte returns: %d\n", tfs_readByte(file2, &read));
	printf("The byte we got back is '%c'\n", read);
	
	printf("Let's use tfs_seek to read a few more to see what is stored in that file now...\n");
	for (test = 0; test < 7; test++) {
		tfs_seek(file2, test);
		printf("tfs_readByte returns: %d\n", tfs_readByte(file2, &read));
		printf("'%c'\n", read);
	}
	
	printf("Oh we must've messed up Barbara's name...\n");
	
	printf("Let's defrag the disk...\n");
	printf("Before: \n\n");
	tfs_readdir();
	tfs_displayFragments();
	tfs_defrag();
	printf("After: \n\n");
	tfs_readdir();
	tfs_displayFragments();

	
	printf("Let's create a new fs using tfs_mkfs...\n");
	printf("tfs_mkfs('TinyFSDisk2') returns: %d\n", tfs_mkfs("TinyFSDisk2", DEFAULT_DISK_SIZE));
	tfs_mount("TinyFSDisk2");
	printf("Let's see what it looks like...\n");
	tfs_displayFragments();

	printf("Now let's open the old one to verify that it is still the same\n");

	tfs_mount("TinyFSDisk");
	tfs_readdir();
	tfs_displayFragments();

	printf(">>> Finish TinyFsDemo program\n");
	return 0;
} 
