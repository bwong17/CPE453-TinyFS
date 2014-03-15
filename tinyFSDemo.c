#include <stdio.h>
#include "libTinyFS.h"

int main(void) {
    
    int file, file1;
    char buffer[BLOCKSIZE] = {"Andrew and Barbara"};
    
    printf("%d\n",tfs_mkfs("test/test2",BLOCKSIZE));

    printf("%d\n", tfs_mount("test/test2"));

    printf("%d\n",file = tfs_openFile("test/test2"));
    //printf("%d\n",file1 = tfs_openFile("test/test1"));

   // printf("%d\n",tfs_closeFile(file1));
    //printf("%d\n",tfs_closeFile(file));
   // printf("%d\n",tfs_closeFile(file));


    printf("%d\n",tfs_writeFile(file, buffer, sizeof(buffer)));
    //printf("%d\n",tfs_closeFile(file));
    return 0;
} 
