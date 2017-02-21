/* Joseph Teague & Clarence Jackson
 * CS560 Lab 1: The File System     */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "fs.h"
#include "shell.h"

#define FSNAME "fs_container"
#define FSFLAGS S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

int initialize();

struct fs *fs = NULL;

int main(int argc, char **argv) {
    initialize();
    return 0;
}

int initialize() {
    off_t pos;

    fs = malloc(sizeof(struct fs));

    listen();
    if(access(FSNAME, F_OK) != -1) {
        printf("file system exists: ");
        if((fs->fd = open(FSNAME, O_RDWR, FSFLAGS)) < 0) {
            printf("ERROR! %d\n", errno);
            return -1;
        }
        printf("fd %d\n", fs->fd);
    } else {
        printf("file system does not exist. Creating it. ");
        if((fs->fd = open(FSNAME, O_RDWR | O_CREAT, FSFLAGS)) < 0) {
            printf("ERROR!\n");
            return -1;
        }
        printf("fd %d\n", fs->fd);
        pos = lseek(fs->fd, (off_t)DISK_SIZE - 1, SEEK_SET);
        printf("position: %lli\n", pos);
        write(fs->fd, "^", 1);
        pos = lseek(fs->fd, 0, SEEK_SET);
        printf("position: %lli\n", pos);
    }

    printf("inode size: %d\n", (int)sizeof(struct inode));
    printf("data size: %d\n", (int)sizeof(fs->data));
    printf("num data blocks: %d\n", (int)sizeof(fs->data) / BLOCK_SIZE);
    printf("leftover data: %d\n", (int)sizeof(fs->data) % BLOCK_SIZE);

    printf("\ndirectory entry size: %d\n", (int)sizeof(struct dir_entry));
    printf("entries per data block: %d\n", (int)BLOCK_SIZE / sizeof(struct dir_entry));
    close(fs->fd);
    return 0;
}
