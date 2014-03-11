#include <stdio.h>
#include <stdlib.h>
#include "libDisk.h"
#include "libTinyFS.h"
#include "TinyFS_errno.h"

static char *disk_mount = NULL;

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
