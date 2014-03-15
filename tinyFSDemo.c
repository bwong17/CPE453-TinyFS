#include <stdio.h>
#include "libTinyFS.h"

int main(void) {
    int file, file1;
    char buffer[BLOCKSIZE];
    
    printf("%d\n",tfs_mkfs("test/test1",BLOCKSIZE));
    
    printf("%d\n", tfs_mount("test/test1"));

   
    printf("%d\n",file = tfs_openFile("test/test1"));
    printf("%d\n",file1 = tfs_openFile("test/test1"));
    
	 printf("%d\n",tfs_closeFile(file1));
	 printf("%d\n",tfs_closeFile(file));
	 printf("%d\n",tfs_closeFile(file));
	 
    //printf("%d\n",tfs_closeFile(file));
    
    return 0;
} 
