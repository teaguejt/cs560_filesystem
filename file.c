/* Joe Teague and Clarence Jackson
 * CS560 Lab 1: The File System
 * file.c - contains the functions for file operations
 */

#include <string.h>
#include <stdio.h>
#include "file.h"
#include "fs.h"
#include <unistd.h>

struct descriptor *get_fd(int index){
    struct descriptor *file = NULL;
    if(fs.files[index]->mode != NODE_MODE_UNUSED)
        file = fs.files[index];
    return file;
}

int file_write(int fd, char *string){
    
	if(access(FSNAME, F_OK) == -1){
		printf("Must create filesystem before operating on files.\n");
		return -1;
	}
	
	/*Checking if fd in open file table
	 * and if flag is set to read.*/
	struct descriptor *file = get_fd(fd);
    if(!file){
        printf("No file assigned file descriptor %d.\n",fd);
        return -1;
    }
    else if(file->flag != 'w'){
        printf("Fd %d is not open for writing.\n",fd);
        return -1;
    }

    lseek(fs.fd, file->node_ptr->blocks[0] + file->offset, SEEK_SET);
    int x = write(fs.fd, string, strlen(string));
	write(fs.fd, "\0", 1);
	file->node_ptr->size += x - (file->node_ptr->size - file->offset) + 1;
	printf("Wrote %d bytes to file descriptor %d.\n",x+1,fd);
    file->offset += x;
    return x;
}

char *file_read(int fd, int size){

    if(access(FSNAME, F_OK) == -1){
		printf("Must create filesystem before operating on files.\n");
		return NULL;
	}

	/*Checking if fd in open file table
	 * and if flag is set to read.*/
	struct descriptor *file = get_fd(fd);
	if(!file){
        printf("No file assigned file descriptor %d.\n",fd);
        return NULL;
    }
    else if(file->flag != 'r'){
        printf("Fd %d is not open for reading.\n");
        return NULL;
    }
	
	/*Case where size = 0 or reading at eof.*/
	if(file->offset == file->node_ptr->size)
		return NULL;

	/*Case where attempting to read more than file size.*/
	if(size > file->node_ptr->size - 1)
		size = file->node_ptr->size -1;
    
	char *string;
    lseek(fs.fd, file->node_ptr->blocks[0] + file->offset, SEEK_SET);
    int x = read(fs.fd, string, size);
    file->offset += x;
	printf("string length %d",strlen(string));
    return string;
}

int file_seek(int fd, int offset){
    
	if(access(FSNAME, F_OK) == -1){
		printf("Must create filesystem before operating on files.\n");
		return -1;
	}
	
	/*Checking if fd in open file table*/
	struct descriptor *file = get_fd(fd);
    if(!file){
       printf("No file assigned file descriptor %d.\n",fd);
       return -1;
    }

	/*Case where offset is bigger than file or file is empty*/
	if(offset > file->node_ptr->size - 1)
		offset = file->node_ptr->size -1;
	if(file->node_ptr->size == 0)
	    offset = 0;

    file->offset = offset;
    return file->offset;
}

int file_close(int fd){
    
	if(access(FSNAME, F_OK) == -1){
		printf("Must create filesystem before operating on files.\n");
		return -1;
	}
  
	/*Checking if fd in open file table*/
	struct descriptor *file = get_fd(fd);
    if(!file){
       printf("No file assigned file descriptor %d.\n",fd);
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

    if(access(FSNAME, F_OK) == -1){
		printf("Must create filesystem before operating on files.\n");
		return -1;
	}
	
	/*Making sure name and flag are in correct format.*/
	if(strlen(name) > MAX_FILE_LENGTH){
		printf("Filename must be 255 charcters or less.\n");
		return -1;
	}
	if(flag != 'r' && flag != 'w'){
		printf("Flag must be either 'r' for read or 'w' for write.\n");
		return -1;
	}

	/*Attemting to find the file, if it does not exist
	 * make anew file.*/
    if(!find_file(name)){
        file = create_file(name);
    }else{
        file = find_file(name);
    }
    
	/*Finding a free file descriptor.*/
    while(fs.files[i]->mode != NODE_MODE_UNUSED){
        i++;
		if(i > 1023){
			printf("Open file limit met. Please close an open file to open a new one.\n");
			return -1;
		}
    }
    fs.files[i]->mode = NODE_MODE_FILE;
    fs.files[i]->flag = flag;
    fs.files[i]->offset = 0;
    fs.files[i]->node_ptr = file;
    return 0;
}
