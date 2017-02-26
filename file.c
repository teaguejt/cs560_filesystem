/* Joe Teague and Clarence Jackson
 * CS560 Lab 1: The File System
 * file.c - contains the functions for file operations
 */

#include <string.h>
#include <stdio.h>
#include "file.h"
#include "fs.h"

extern struct fs fs;

struct descriptor *get_fd(int index){
    struct descriptor *file = NULL;
    if(fs.files[index]->mode != NODE_MODE_UNUSED)
        file = fs.files[index];
    return file;
}

int file_write(int fd, char *string){
    struct descriptor *file = get_fd(fd);
    if(!file){
        printf("No file assigned file descriptor %d",fd);
        return -1;
    }
    else if(file->flag != 'w'){
        printf("File is not open for writing");
        return -1;
    }
    int offset = file->offset; 
    lseek(fs.fd, file->node_ptr->blocks[0] + offset, SEEK_SET);
    int x = write(fs.fd, string, strlen(string));
    file->offset += x;
    return x;
}

int file_read(int fd, int size){
    struct descriptor *file = get_fd(fd);
    if(!file){
        printf("No file assigned file descriptor %d",fd);
        return -1;
    }
    else if(file->flag != 'r'){
        printf("File is not open for reading");
        return -1;
    }
    int offset = file->offset; 
    char string[size];
    printf("offset is %d",offset);
    lseek(fs.fd, file->node_ptr->blocks[0] + offset, SEEK_SET);
    int x = read(fs.fd, string, size);
    file->offset += x;
    printf("%s\n",string);
    return x;
}

int file_seek(int fd, int offset){
    struct descriptor *file = get_fd(fd);
    if(!file){
       printf("No file assigned file descriptor %d",fd);
       return -1;
    }
    file->offset = offset;
    return file->offset;
}

int file_close(int fd){
    struct descriptor *file = get_fd(fd);
    if(!file){
       printf("No file assigned file descriptor %d",fd);
       return -1;
    }
    file->mode = NODE_MODE_UNUSED;
    file->flag = -1;
    file->offset = -1;
    file->node_ptr = NULL;
    return 0;
}

int file_open(char *name, char flag){
    struct inode *file;
    int i = 0;

    if(!find_file(name)){
        file = create_file(name);
    }
    else{
        file = find_file(name);
    }
    
    while(fs.files[i]->mode != NODE_MODE_UNUSED){
        i++;
    }
    if(i > 1023){
        printf("Open file limit exceeded. Please close an open file.");
    }
    fs.files[i]->mode = NODE_MODE_FILE;
    fs.files[i]->flag = flag;
    fs.files[i]->offset = 0;
    fs.files[i]->node_ptr = file;
    return 0;
}
