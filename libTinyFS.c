#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>

#include "libDisk.h"
#include "libTinyFS.h"
#include "TinyFS_errno.h"

static char *disk_mount = NULL;
static drt_t *dynamicResourceTable = NULL;

int getDrtLength(drt_t *head) {
	int i = 0;

	while(head->next) {
		head = head->next;
		i++;
	}

	return i;
}


int removeDrtNode(fileDescriptor fd) {
	drt_t *temp = dynamicResourceTable;
	drt_t *prev = temp;
	
	// if drt is NULL then there are no files open
	if (!dynamicResourceTable) 
		return ERR_FILENOTOPEN;

	// search for id == fd
	while (temp->next && (temp->id != fd)) {
		prev = temp;
		temp = temp->next;
	}

	// if we are at the end of the node and it doesnt match
	// then file isnt open
	if (!temp->next && temp->id != fd)
		return ERR_FILENOTOPEN;

	// if we are at the root node then we need to change the root node
	if (temp == dynamicResourceTable)
		// if there is more than just the root node
		if (dynamicResourceTable->next)
			dynamicResourceTable = dynamicResourceTable->next;
		else
			dynamicResourceTable = NULL;
        else
		// if we are not at the end of the list
		if (temp->next)
			prev->next = temp->next;
		else
			prev->next = NULL;
        
	// temp is always pointing at the node that we want to remove from the list
	free(temp);

	return 1;
}

drt_t *addDrtNode(char* fileName, drt_t *head) {
	drt_t *node = calloc(1, sizeof(drt_t));
	node->fileName = fileName;

	while (head->next)
		head = head->next;
	
	node->id = head->id + 1;
	head->next = node;
	return node;
}

int tfs_mkfs(char *filename, int nBytes){

	int i, diskNum;
	char* templateBlk = calloc(1, BLOCKSIZE);

	templateBlk[0] = 4;
	templateBlk[1] = 0x45;

	diskNum = openDisk(filename, nBytes);

	if(diskNum < 0)
		return ERR_MKDSK;

	for(i = 0; i < BLOCKSIZE; i++)
		writeBlock(diskNum, i, templateBlk);

	/* SUPER BLOCK */
	templateBlk[0] = 1;
	templateBlk[2] = 1;
	writeBlock(diskNum, 0, templateBlk);

	// Set up root inode block
	// Unneccessary for this assignment
	/*templateBlk[0] = 2;
	  templateBlk[2] = 2;
	  writeBlock(diskNum, 1, templateBlk);
	 */
	free(templateBlk);
	return 1;
}

int tfs_mount(char *filename) {
	int i, diskNum;

	if (disk_mount)
		tfs_unmount();

	char buff[BLOCKSIZE];
	diskNum = openDisk(filename, 0);
	readBlock(diskNum, 0, buff);

	if (buff[1] != 0x45)   
		return ERR_BADFS;

	disk_mount = filename;
	return 1;
}

int tfs_unmount() {
	if(!disk_mount)
		return ERR_FILE_ALREADY_UNMOUNTED;

	disk_mount = NULL;
	return 1;
}

fileDescriptor tfs_openFile(char *name) {
	int fd = ERR_BADFILE;
	char buff[BLOCKSIZE];
	int i = 0;

	// check if we have a mounted disk
	if(disk_mount)
		fd = openDisk(disk_mount, BLOCKSIZE);
	else
		return ERR_FILENOTMOUNTED;

	// find first free location to place an inode block
	for (i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE; i++) {
		if (readBlock(fd, i, buff) < 0)
			return ERR_NOMORESPACE;  
		if (buff[0] == 4)
			break;
	}
	// buff is now a template of the inode block
	// i is the block number of the soon to be inode
	// label it as an inode
	// copy name to front end
	buff[0] = 2;
	memcpy(buff+4, name, strlen(name));
	writeBlock(fd, i, buff);

	// add a new entry in drt that refers to this filename
	// returns a fileDescriptor (temp->id)
	drt_t *temp;
	if (dynamicResourceTable == NULL) {
		temp = calloc(1, sizeof(drt_t));
		temp->fileName = name;
		dynamicResourceTable = temp;
	} else {
		temp = addDrtNode(name, dynamicResourceTable);
	}

	return temp->id;
}

int tfs_closeFile(fileDescriptor FD)
{
	return removeDrtNode(FD);
}

int tfs_writeFile(fileDescriptor FD, char *buffer, int size)
{

    int i,startIndex, endIndex, j, fd, numBlocks, found = 1;
    char buff[BLOCKSIZE];
    char *fileName;
    drt_t *temp = dynamicResourceTable;

    numBlocks = ceil((double)size / (double)BLOCKSIZE);

    temp += FD;
    fileName = temp->fileName;
    
    if(disk_mount)
        fd = openDisk(disk_mount, BLOCKSIZE);
    else
	return ERR_FILENOTMOUNTED;
    
    /* NOTE: super block not set correctly */

    /* find first free "numBlocks" block occurences to write to */
    for (i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE; i++) {
        if (readBlock(fd, i, buff) < 0)
            return ERR_NOMORESPACE;
        if (buff[0] == 4){
            for(j = i; j < i+numBlocks; j++){
                if (readBlock(fd, j, buff) < 0)
                    return ERR_NOMORESPACE;  
                if (buff[0] != 4)
                    found = 0;
            }
            if(found)
                break;
        }
    }
    if(!found)
        return ERR_NOFREEBLOCKFOUND;

    startIndex = i;
    endIndex = j;
    
    /* setting template to make (write) file extents*/
    buff[0] = 3;
    buff[1] = 0x45;
    
    for(i = startIndex; i <= endIndex; i++){
        if(i != endIndex)
            buff[2] = i+1;
        else
            buff[2] = 0;

        strncpy(buff+4,buffer,BLOCKSIZE-4);
        writeBlock(fd, i, buff); 
    }
    
    /* inode update (1st block occurrence and size) */
    for(i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE || !found; i++){
        if(readBlock(fd, i, buff) < 0)
            return ERR_NOMORESPACE;
        if(buff[0] == 2){
            if(!strcmp(fileName, buff+4)){
                buff[2] = startIndex;
                buff[13] = (numBlocks & 0xFF00)>>8;
                buff[14] = numBlocks & 0x00FF;
                writeBlock(fd, i, buff);
                found = 1;
            }
        }
    }
    if(!found)
        return ERR_NOINODEFOUND;

    return 1;
}

int tfs_deleteFile(fileDescriptor FD)
{
	return 0;
}

int tfs_readByte(fileDescriptor FD, char *buffer)
{
	return 0;
}

int tfs_seek(fileDescriptor FD, int offset)
{
	return 0;
}
