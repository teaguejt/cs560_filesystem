/* Joseph Teague & Clarence Jackson
 * CS560 Lab 1: The File System     */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "fs.h"
#include "shell.h"

int main(int argc, char **argv) {
    if(fs_init() != 0) {
        printf("fs: error initializing filesystem. Exiting.\n");
        return -1;
    }

    listen();

    fs_close();
    return 0;
}
