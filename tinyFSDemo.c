#include <stdio.h>
#include "libTinyFS.h"

int main(void) {
    int file;
    char buffer[BLOCKSIZE];
    
    printf("%d\n",tfs_mkfs("test/test1",BLOCKSIZE));
    
    printf("%d\n", tfs_mount("test/test1"));

    printf("%d\n",file = tfs_openFile("test/test1"));

    printf("%d\n",tfs_closeFile(file));
    return 0;
} 
