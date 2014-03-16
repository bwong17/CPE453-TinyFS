#include <stdio.h>
#include "libTinyFS.h"

int main(void) {
    
    int file1, file2;
    char buffer[BLOCKSIZE] = {"Andrew and Barbara"};
    
    printf("TFS Make FS %d\n",tfs_mkfs("tinyFSDisk",DEFAULT_DISK_SIZE));

    printf("TFS Mount %d\n", tfs_mount("tinyFSDisk"));

    printf("TFS Open File 1: %d\n",file1 = tfs_openFile("test1"));
    printf("TFS Open File 2: %d\n", file2 = tfs_openFile("test2"));
    printf("TFS Write File 1: %d\n",tfs_writeFile(file1, buffer, sizeof(buffer)));
    printf("TFS Write File 2: %d\n",tfs_writeFile(file2, buffer, sizeof(buffer)));

    printf("TFS Delete File 1: %d\n",tfs_deleteFile(file1));
    
    return 0;
} 
