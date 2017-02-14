/* Joseph Teague & Clarence Jackson
 * CS560 Lab 1: The File System     */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "fs.h"

#define FSNAME "fs_container"

int initialize();

struct fs *fs = NULL;

int main(int argc, char **argv) {
    initialize();
    return 0;
}

int initialize() {
    fs = malloc(sizeof(struct fs));

    if(access(FSNAME, F_OK) != -1) {
        printf("file system exists: ");
        fs->fd = open(FSNAME, O_RDWR);
    } else {
        printf("file system does not exist. Creating it. ");
        fs->fd = open(FSNAME, O_RDWR | O_CREAT);
        lseek(fs->fd, 100<<20, SEEK_SET);
        write(fs->fd, "^", 1);
        lseek(fs->fd, 0, SEEK_SET);

    }
}
