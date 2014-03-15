#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

drt_t *addDrtNode(drt_t *head) {
    drt_t *node = calloc(1, sizeof(drt_t));
    while (head->next)
        head = head->next;
    head->next = node;
    head->next->id = head->id + 1;
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
    
    // Set up inode block
    templateBlk[0] = 2;
    templateBlk[2] = 2;
    writeBlock(diskNum, 1, templateBlk);

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

fileDescriptor tfs_openFile(char *name)
{
    int fd = ERR_BADFILE;
    char buff[BLOCKSIZE];
    int i = 0;

    if(disk_mount)
        fd = openDisk(disk_mount, BLOCKSIZE);
    else
        return ERR_FILENOTMOUNTED;
    
    for (i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE; i++) {
        if (readBlock(fd, i, buff) < 0)
            return ERR_NOMORESPACE;  
        if (buff[0] == 2 && !strcmp(buff+4, name))
            break;
    }

    drt_t *temp;
    if (dynamicResourceTable == NULL) {
        temp = calloc(1, sizeof(drt_t));
        dynamicResourceTable = temp;
    } else {
        temp = addDrtNode(dynamicResourceTable);
    }

    for (i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE; i++) {
        readBlock(fd, i, temp->data + i * BLOCKSIZE);
    }

    return temp->id;
}

int tfs_closeFile(fileDescriptor FD)
{
    //return close(FD);
    return 0;
}

int tfs_writeFile(fileDescriptor FD, char *buffer, int size)
{
    return 0 ;
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


