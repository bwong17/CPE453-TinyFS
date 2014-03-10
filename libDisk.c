#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "libDisk.h"
#include "libTinyFS.h"

int openDisk(char *filename, int nBytes)
{
	int file;
	int i;
	char buffer[nBytes];

	file = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if(file) {
		if(nBytes > 0) {
			for(i = 0; i < nBytes; i++)
				buffer[i] = 0x41;
			write(file, buffer, nBytes);
		}
		if(!nBytes)
			return -1;
	} else
		return -1;
	return (int)file;
}

int readBlock(int disk, int bNum, void *block)
{
	int bytesRead;
	bytesRead = -1;

	lseek(disk, 0, SEEK_SET);

	if ((bytesRead = pread(disk, block, BLOCKSIZE, bNum * BLOCKSIZE)) == -1) {
		printf("Error:%s %d\n", strerror(errno), disk);
		return -1;
	} else
		return 0;
}

int writeBlock(int disk, int bNum, void *block)
{
	int bytesWritten;
	bytesWritten = -1;

	lseek(disk, 0, SEEK_SET);

	if((bytesWritten = pwrite(disk, block, BLOCKSIZE, bNum)) == -1) {
		printf("Error:%s %d\n", strerror(errno), disk);
		return -1;
	} else
		return 0;
}
