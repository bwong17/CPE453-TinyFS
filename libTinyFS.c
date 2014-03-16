#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>

#include "libDisk.h"
#include "libTinyFS.h"
#include "TinyFS_errno.h"

#define DEBUG 0

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

        if(DEBUG)
            printf("Removed a node from dynamic resource table\n");
	// temp is always pointing at the node that we want to remove from the list
	free(temp);

	return 1;
}

drt_t *addDrtNode(char* fileName, drt_t *head, char isRW) {

	drt_t *node = calloc(1, sizeof(drt_t));
	node->fileName = fileName;

	while (head->next)
		head = head->next;

	node->id = head->id + 1;
	node->access = isRW;
	head->next = node;

        if(DEBUG)
            printf("Created new node with id %d\n",node->id);
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

        if(DEBUG)
            printf("Made File System with super block\n");

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
	if (diskNum < 0)
		return ERR_BADFS;

	readBlock(diskNum, 0, buff);

	if (buff[1] != 0x45)   
		return ERR_BADFS;

	disk_mount = filename;

        if(DEBUG)
            printf("Mounted disk %s\n",disk_mount);
        close(diskNum);
	return 1;
}

int tfs_unmount() {
	if(!disk_mount)
		return ERR_FILE_ALREADY_UNMOUNTED;

        if(DEBUG)
            printf("Unmounted disk %s\n",disk_mount);
	disk_mount = NULL;
	return 1;
}

int checkIfAlreadyOpen(char *name) {
	drt_t *cur = dynamicResourceTable;
	while (cur) {
		if (!strcmp(name, cur->fileName))
			return cur->id;
		else
			cur = cur->next;
	}
	return -1;
}

fileDescriptor tfs_openFile(char *name) {
	int fd = ERR_BADFILE;
	char buff[BLOCKSIZE];
	int i = 0;

	// check if we have a mounted disk
	if(disk_mount)
		if (checkIfAlreadyOpen(name) >= 0)
			return checkIfAlreadyOpen(name);
		else
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
	buff[0] = 2; // set as inode block
	buff[3] = 1; // set as RW by default
	memcpy(buff+4, name, strlen(name));
	writeBlock(fd, i, buff);

        if(DEBUG)
             printf("Created inode block with filename %s\n",buff+4);

	// add a new entry in drt that refers to this filename
	// returns a fileDescriptor (temp->id)
	drt_t *temp;
	if (dynamicResourceTable == NULL) {
		temp = calloc(1, sizeof(drt_t));
		temp->fileName = name;
		temp->access = 1;
		dynamicResourceTable = temp;
        
        if(DEBUG)
            printf("Created first dynamic resource table node with filename %s\n",temp->fileName);

	} else {
		temp = addDrtNode(name, dynamicResourceTable, buff[3]);
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

	if(disk_mount)
		fd = openDisk(disk_mount, 0);
	else
		return ERR_FILENOTMOUNTED;

	numBlocks = ceil((double)size / (double)BLOCKSIZE);

	while(temp){
		if(temp->id == FD){
			break;
		}
		temp = temp->next;
	}
	if (!temp)
		return ERR_BADFILE;

	if(!temp->access)
		return ERR_READONLY;
	fileName = temp->fileName;


	/* find first free "numBlocks" block occurences to write to */
	for (i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE; i++) {
		if (readBlock(fd, i, buff) < 0)
			return ERR_NOMORESPACE;
		if (buff[0] == 4){
			for(j = i; j < i+numBlocks-1; j++){
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
	found = 0;

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

        if(DEBUG)
            printf("Write to free blocks starting at index %d and ending at %d. Wrote to the file extent -> %s\n",startIndex, endIndex,buff+4);

	/* inode update (1st block occurrence and size) */
	for(i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE && !found; i++){
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
	
        if(DEBUG)
            printf("Updated inode block to point to index %d and have size %d %d\n",buff[2],buff[13],buff[14]);
        
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
	
	if(disk_mount)
		fd = openDisk(disk_mount, 0);
	else
		return ERR_FILENOTMOUNTED;
	

	while(temp){
		if(temp->id == FD){
			break;
		}
		temp = temp->next;
	}
	
	if (!temp)
		return ERR_BADFILE;

	fileName = temp->fileName;

	/* looking for inode block */
	for(i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE && !found; i++){
		if(readBlock(fd, i, buff) < 0)
			return ERR_NOMORESPACE;
		if(buff[0] == 2){
			if(!strcmp(fileName, buff+4)){
				if (buff[3] == 0)
					return ERR_READONLY;
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

        if(DEBUG)
            printf("Deleted inode block at index %d\n",i);
	/*deleting file extents*/
	for(i = firstBlock; i <= firstBlock + numBlocks; i++) {
		writeBlock(fd, i, buff);    
	}

        if(DEBUG)
            printf("Deleted file extents from index %d to %d\n",firstBlock, firstBlock+numBlocks);
	removeDrtNode(FD);
	close(fd);

	return 1;
}

int tfs_readByte(fileDescriptor FD, char *buffer)
{
	int i, fd, size, firstBlock, numBlocks, currBlock, tempFP;
	int found = 0;
	char buff[BLOCKSIZE];
	char *fileName;
	drt_t *temp = dynamicResourceTable;
	
	if(disk_mount)
		fd = openDisk(disk_mount, 0);
	else
		return ERR_FILENOTMOUNTED;
	
	while(temp){
		if(temp->id == FD){
			break;
		}
		temp = temp->next;
	}

	if (!temp)
		return ERR_BADFILE;

	fileName = temp->fileName;

	/* looking for inode block */
	for(i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE && !found; i++){
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
	if (buff[0] != 3)
		return buff[0];//ERR_REACHEDEOF;
	*buffer = buff[tempFP+4];
        temp->fileptr++;
        
        if(DEBUG)
            printf("Read Byte %c. Pointer now at index %d\n",*buffer,temp->fileptr);
	close(fd);
	return 1;
}

int tfs_seek(fileDescriptor FD, int offset)
{
	if(!disk_mount)
		return ERR_FILENOTMOUNTED;
	
	drt_t *temp = dynamicResourceTable;

	while(temp){
		if(temp->id == FD){
			break;
		}
		temp = temp->next;
	}

	if (!temp)
		return ERR_BADFILE;

	temp->fileptr = offset;

        if(DEBUG)
            printf("Seeked pointer to index %d\n",temp->fileptr);
	return 1;
}

int tfs_rename(fileDescriptor FD, char *name){
	int i, fd,inodeLoc;
	int found = 0;
	char buff[BLOCKSIZE];
	char *fileName;
	drt_t *temp = dynamicResourceTable;
	
	if(disk_mount)
		fd = openDisk(disk_mount, 0);
	else
		return ERR_FILENOTMOUNTED;
	
	while(temp){
		if(temp->id == FD){
			break;
		}
		temp = temp->next;
	}

	if (temp == NULL)
		return ERR_BADFILE;

	fileName = temp->fileName;

	/* change name in inode block */

	for(i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE && !found; i++){
		if(readBlock(fd, i, buff) < 0)
			return ERR_NOMORESPACE;
		if(buff[0] == 2){
			if(!strcmp(fileName, buff+4)){
				found = 1;
				break;
			}
		}
	}
	inodeLoc = i;
	if(!found)
		return ERR_NOINODEFOUND;

        /* looking for inode block */
	if(DEBUG)
	    printf("Inode block name changes from %s to ",buff+4);
	strcpy(buff+4,name);
	writeBlock(fd,inodeLoc,buff);
	if(DEBUG)
            printf("%s\n",buff+4);

	/* change name in drt */
	if(DEBUG)
            printf("Drt name changed from %s to ",temp->fileName);
	temp->fileName = name;
	if(DEBUG)
            printf("%s\n",temp->fileName);

	close(fd);
	return 1;
}

int tfs_readdir(){

	drt_t *temp = dynamicResourceTable;

	printf("***** List of Files and Directories *****\n");

	while(temp){
		printf("%s\n",temp->fileName);
		temp = temp->next;
	}
	printf("*********************************\n");

	return 1;
}

int tfs_makeRO(char *name) {
	
	int found = 0;
	int i = 0;
	int fd;
	char buff[BLOCKSIZE];

	if(disk_mount)
		fd = openDisk(disk_mount, 0);
	else
		return ERR_FILENOTMOUNTED;

	// Loop over inode blocks
	for(i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE && !found; i++){
		if(readBlock(fd, i, buff) < 0)
			return ERR_NOMORESPACE;
		if(buff[0] == 2){
			if(!strcmp(name, buff+4)){
				found = 1;
				buff[3] = 0; // Set RO byte
				writeBlock(fd, i, buff);
				break;
			}
		}
	}

	if(!found)
		return ERR_NOINODEFOUND;
	else {
	    if(DEBUG)	
                printf("%s access changes to Read-Only(%d)\n",name, buff[3]);
		return 1;
	}

}

int tfs_makeRW(char *name) {

	int found = 0;
	int i = 0;
	int fd;
	char buff[BLOCKSIZE];

	if(disk_mount)
		fd = openDisk(disk_mount, 0);
	else
		return ERR_FILENOTMOUNTED;

	// Loop over inode blocks
	for(i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE && !found; i++){
		if(readBlock(fd, i, buff) < 0)
			return ERR_NOMORESPACE;
		if(buff[0] == 2){
			if(!strcmp(name, buff+4)){
				found = 1;
				buff[3] = 1; // Set RW byte
				writeBlock(fd, i, buff);
				break;
			}
		}
	}

	if(!found)
		return ERR_NOINODEFOUND;
	else {
	    if(DEBUG)	
                printf("%s access changes to Read-Write(%d)\n",name, buff[3]);
		return 1;
	}
}

int tfs_writeByte(fileDescriptor FD, unsigned char data) {
	int i, fd, size, firstBlock, numBlocks, currBlock, tempFP;
	int found = 0;
	char buff[BLOCKSIZE];
	char *fileName;
	drt_t *temp = dynamicResourceTable;
	
	if(disk_mount)
		fd = openDisk(disk_mount, 0);
	else
		return ERR_FILENOTMOUNTED;

	while(temp){
		if(temp->id == FD){
			break;
		}
		temp = temp->next;
	}

	if (!temp)
		return ERR_BADFILE;

	if(!temp->access)
		return ERR_READONLY;

	fileName = temp->fileName;


	/* looking for inode block */
	for(i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE && !found; i++){

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
	buff[tempFP+4] = data;
	writeBlock(fd,currBlock+firstBlock,buff);
	if(DEBUG)
            printf("Wrote %c to byte at block %d\n",buff[tempFP+4],currBlock+firstBlock);
        close(fd);
	return 1;
}

int tfs_displayFragments() {

	int i, fd, count = 0;
	char buff[BLOCKSIZE];

	if(disk_mount)
		fd = openDisk(disk_mount, 0);
	else
		return ERR_FILENOTMOUNTED;
	for(i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE; i++){

		if(readBlock(fd, i, buff) < 0)
			return ERR_NOMORESPACE;

		if(buff[0] == 1){
			printf("|S|");
			count++;
		}
		else if(buff[0] == 2){
			printf("|I|");
			count++;
		}
		else if(buff[0] == 3){
			printf("|D|");
			count++;
		}
		else if(buff[0] == 4){
			printf("| |");
			count++;
		}
		if(count == 4) {
			printf("\n");
			count = 0;
		}
		else
			printf(" -> ");
	}
	return 1;
}
