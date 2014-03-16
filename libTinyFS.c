#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>

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

	free(templateBlk);
        close(diskNum);
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
        close(diskNum);
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
		fd = openDisk(disk_mount, 0);
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
        close(fd);

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

    while(temp){
        if(temp->id == FD){
            break;
        }
        temp = temp->next;
    }
    fileName = temp->fileName;
    
    if(disk_mount)
        fd = openDisk(disk_mount, 0);
    else
	return ERR_FILENOTMOUNTED;
    
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
    close(fd);

    return 1;
}

int tfs_deleteFile(fileDescriptor FD)
{
    int i, fd, size, firstBlock, numBlocks;
    int found = 0;
    char buff[BLOCKSIZE];
    char *fileName;
    drt_t *temp = dynamicResourceTable;

    while(temp){
        if(temp->id == FD){
            break;
        }
        temp = temp->next;
    }
    fileName = temp->fileName;
    if(disk_mount)
        fd = openDisk(disk_mount, 0);
    else
	return ERR_FILENOTMOUNTED;

    /* looking for inode block */
    for(i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE || !found; i++){
        if(readBlock(fd, i, buff) < 0)
            return ERR_NOMORESPACE;
        if(buff[0] == 2){
            if(!strcmp(fileName, buff+4)){
                found = 1;
                firstBlock = buff[2];
                size = (buff[13] << 8) || buff[14]; 
                numBlocks = (int)ceil((double)size / (double)BLOCKSIZE);
                break;
            }
        }
    }
    if(!found)
        return ERR_NOINODEFOUND;
    
    buff[0] = 4;
    
    /*deleting inode */
    writeBlock(fd, i,buff);

    /*deleting file extents*/
    for(i = firstBlock; i <= firstBlock + numBlocks; i++) {
        writeBlock(fd, i, buff);    
    }

    removeDrtNode(FD);
    close(fd);

    /* checking for Blocks in FS 
    for(i = 0; i < 10; i++){
        readBlock(fd, i, buff);
        printf("block %d : %d\n",i,buff[0]);
    }
    */
    return 1;
}

int tfs_readByte(fileDescriptor FD, char *buffer)
{
    int i, fd, size, firstBlock, numBlocks, currBlock, tempFP;
    int found = 0;
    char buff[BLOCKSIZE];
    char *fileName;
    drt_t *temp = dynamicResourceTable;

    while(temp){
        if(temp->id == FD){
            break;
        }
        temp = temp->next;
    }
    fileName = temp->fileName;
    if(disk_mount)
        fd = openDisk(disk_mount, 0);
    else
	return ERR_FILENOTMOUNTED;

    /* looking for inode block */
    for(i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE || !found; i++){
        if(readBlock(fd, i, buff) < 0)
            return ERR_NOMORESPACE;
        if(buff[0] == 2){
            if(!strcmp(fileName, buff+4)){
                found = 1;
                firstBlock = buff[2];
                size = (buff[13] << 8) || buff[14]; 
                numBlocks = (int)ceil((double)size / (double)BLOCKSIZE);
                break;
            }
        }
    }
    if(!found)
        return ERR_NOINODEFOUND;
    
    currBlock = (int)floor(((double)temp->fileptr+1) / (double)BLOCKSIZE);
    tempFP = temp->fileptr - (BLOCKSIZE * currBlock);
    readBlock(fd,currBlock+firstBlock,buff); 
    *buffer = buff[tempFP+4];
    temp->fileptr++;
    close(fd);
    return 1;
}

int tfs_seek(fileDescriptor FD, int offset)
{
    drt_t *temp = dynamicResourceTable;

    while(temp){
        if(temp->id == FD){
            break;
        }
        temp = temp->next;
    }
    temp->fileptr = offset;

    return 1;
}

int tfs_rename(fileDescriptor FD, char *name){

    int i, fd;
    int found = 0;
    char buff[BLOCKSIZE];
    char *fileName;
    drt_t *temp = dynamicResourceTable;

    while(temp){
        if(temp->id == FD){
            break;
        }
        temp = temp->next;
    }
    fileName = temp->fileName;
    
    /* change name in inode block */
    if(disk_mount)
        fd = openDisk(disk_mount, 0);
    else
	return ERR_FILENOTMOUNTED;

    for(i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE || !found; i++){
        if(readBlock(fd, i, buff) < 0)
            return ERR_NOMORESPACE;
        if(buff[0] == 2){
            if(!strcmp(fileName, buff+4)){
                found = 1;
                break;
            }
        }
    }
    if(!found)
        return ERR_NOINODEFOUND;
   
    //printf("inode block name changes from %s to ",buff+4);
    strcpy(buff+4,name);

    //printf("%s\n",buff+4);

    /* change name in drt */
    //printf("drt name changed from %s to ",temp->fileName);
    temp->fileName = name;
    //printf("%s\n",temp->fileName);
    
    close(fd);
    return 1;
}

int tfs_readdir(){

    drt_t *temp = dynamicResourceTable;

    printf("***** Files and Directories *****\n");

    while(temp){
        printf("%s\n",temp->fileName);
        temp = temp->next;
    }
    printf("*********************************\n");

    return 0;
}
