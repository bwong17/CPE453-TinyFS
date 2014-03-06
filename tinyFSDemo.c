#include "libDisk.h"
#include "libTinyFS.h"

#include <stdio.h>

int main(void) {
    int file;
    char buffer[BLOCKSIZE];
    
    file = openDisk("test",256);
    
    printf("FD in main: %d\n", file);
    readBlock(file, 0, buffer);
    
    printf("%s", buffer);

    return 0;
} 
