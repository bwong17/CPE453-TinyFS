#include <stdio.h>
#include "libTinyFS.h"

int main(void) {
    
    int file1, file2;
    char buffer[BLOCKSIZE] = {"Andrew and Barbara"};
    char read;
    
    printf("TFS Make FS %d\n",tfs_mkfs("tinyFSDisk",DEFAULT_DISK_SIZE));

    printf("TFS Mount %d\n", tfs_mount("tinyFSDisk"));

    printf("TFS Open File 1: %d\n",file1 = tfs_openFile("test1"));
    printf("TFS Open File 2: %d\n", file2 = tfs_openFile("test2"));
    printf("TFS Write File 1: %d\n",tfs_writeFile(file1, buffer, sizeof(buffer)));
   printf("TFS Write File 2: %d\n",tfs_writeFile(file2, buffer, sizeof(buffer)));

   printf("TFS Read Files and Directories: %d\n",tfs_readdir());
    printf("TFS Delete File 1: %d\n",tfs_deleteFile(file1));

   printf("TFS Read Files and Directories: %d\n",tfs_readdir());
    printf("TFS Read Byte File 2: %d\n",tfs_readByte(file2,&read));
    printf("TFS Read Byte File 2: %d\n",tfs_readByte(file2,&read));
    printf("TFS Read Byte File 2: %d\n",tfs_readByte(file2,&read));
    printf("TFS Read Byte File 2: %d\n",tfs_readByte(file2,&read));

    printf("TFS Seek File 2: %d\n",tfs_seek(file2,11));
    printf("TFS Read Byte File 2: %d\n",tfs_readByte(file2,&read));
    printf("TFS Rename File 2: %d\n",tfs_rename(file2, "file1"));
   printf("TFS Read Files and Directories: %d\n",tfs_readdir());
    return 0;
} 
