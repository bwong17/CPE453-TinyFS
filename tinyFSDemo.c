#include <stdio.h>
#include "libTinyFS.h"

int main(void) {
    int file;
    char buffer[BLOCKSIZE];
    
    tfs_mkfs("test/test1",BLOCKSIZE);
    
    tfs_mount("test/test1");

    return 0;
} 
