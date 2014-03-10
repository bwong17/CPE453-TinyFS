#include "libDisk.h"
#include "libTinyFS.h"

#include <stdio.h>

int main(void) {
    int file;
    char buffer[BLOCKSIZE];
    char block[] = {'B','B','B','B','B'};
    
    file = openDisk("test",256);
    
    readBlock(file, 0, buffer);
    printf("%s\n\n", buffer);
    writeBlock(file, 0, block); 
    readBlock(file, 0, buffer);
    printf("%s\n\n", buffer);
	
    return 0;
} 
