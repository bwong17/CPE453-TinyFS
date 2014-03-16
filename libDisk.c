#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "TinyFS_errno.h"
#include "libDisk.h"
#include "libTinyFS.h"

int openDisk(char *filename, int nBytes)
{
	int file;
	int i;
	char buffer[nBytes];
	

	file = !nBytes ? open(filename, O_RDWR, S_IRUSR | S_IWUSR) : open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if(file > 0) {
            if(nBytes > 0) {
                for(i = 0; i < nBytes; i++)
                    buffer[i] = 0;
                write(file, buffer, nBytes);
            }
        } else
            return ERR_BADFILE;
        
        return (int)file;
}

int readBlock(int disk, int bNum, void *block)
{
	int bytesRead;
	bytesRead = -1;

	lseek(disk, 0, SEEK_SET);

	if ((bytesRead = pread(disk, block, BLOCKSIZE, bNum * BLOCKSIZE)) == -1) {
		printf("Error:%s %d\n", strerror(errno), disk);
		return ERR_BADREAD;
	} else
		return 0;
}

int writeBlock(int disk, int bNum, void *block)
{
	int bytesWritten;

	lseek(disk, 0, SEEK_SET);

	if((bytesWritten = pwrite(disk, block, BLOCKSIZE, bNum * BLOCKSIZE)) == -1) {
		printf("Error:%s %d\n", strerror(errno), disk);
		return ERR_BADWRITE;
	} else
		return 0;
}
