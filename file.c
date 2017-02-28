/* Joe Teague and Clarence Jackson
 * CS560 Lab 1: The File System
 * file.c - contains the functions for file operations
 */

#include <string.h>
#include <stdio.h>
#include "file.h"
#include "fs.h"
#include <unistd.h>

/*Struct containing open file info*/
struct descriptor *get_fd(int index){
    struct descriptor *file = NULL;
    if(fs.files[index]->mode != NODE_MODE_UNUSED)
        file = fs.files[index];
    return file;
}


/*Function for writing to files in the fs*/
int file_write(int fd, char *string){
    int i = 0;

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
	if(x == -1){
		printf("Write to fs failed.\n");
		return -1;
	}
	
	if(file->node_ptr->size <= file->offset + x){
		write(fs.fd, '\0', 1);
		file->node_ptr->size += (file->offset + x + 1) - file->node_ptr->size;
		i = 1;
	}
	printf("Wrote %d bytes to file descriptor %d.\n",x+i,fd);
	printf("New filesize: %d.\n",file->node_ptr->size);
    file->offset += x;
    return x;
}


/*Function for reading from files in the fs*/
char *file_read(int fd, int size){
	char *string;
    
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
    
	string = (char*)malloc(sizeof(char)*size);
    lseek(fs.fd, file->node_ptr->blocks[0] + file->offset, SEEK_SET);
    int x = read(fs.fd, string, size);
	if(x == -1){
		printf("Read from fs failed.\n");
		return NULL;
	}
    file->offset += x;
    return string;
}


/*Function for chnging the offset of an open file in the fs*/
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

    printf("Offset is %d for file descriptor %d.\n",offset,fd);
    file->offset = offset;
    return file->offset;
}


/*Function for closing an open file in the fs*/
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

    printf("Closing file descriptor %d.\n",fd);
    file->mode = NODE_MODE_UNUSED;
    file->flag = -1;
    file->offset = -1;
    file->node_ptr = NULL;
    return 0;
}


/*Function for opening a file in the fs*/
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
	printf("Opening %s with flag '%c' on file descriptor %d.\n",name,flag,i);
    fs.files[i]->mode = NODE_MODE_FILE;
    fs.files[i]->flag = flag;
    fs.files[i]->offset = 0;
    fs.files[i]->node_ptr = file;
    return i;
}


/*Function for displaying a files content*/
void file_cat(char *name){
    struct inode *file;
	char* string;

    if(access(FSNAME, F_OK) == -1){
		printf("Must create filesystem before operating on files.\n");
		return;
	}
	
	/*Making sure name is in correct format*/
	if(strlen(name) > MAX_FILE_LENGTH){
		printf("Filename must be 255 charcters or less.\n");
		return;
	}
	
	/*Attemting to find the file, if it does not exist
	 * return.*/
    if(!find_file(name)){
		printf("No file named %s\n.",name);
		return;
    }

    file = find_file(name);
	printf("filesize: %d\n",file->size);
	string = (char*)malloc(sizeof(char)*file->size);
    lseek(fs.fd, file->blocks[0], SEEK_SET);
	int x = read(fs.fd, string, file->size);
	if(x == -1){
		printf("Read from fs failed.");
		return;
	}
	printf("%s\n",string);
}

int file_import(char* src, char* dest){
	char *file;
	long size;
	FILE *fd = fopen(src, "rb");
	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	rewind(fd);
	file = malloc(size * (sizeof(char)));
	fread(file, sizeof(char), size, fd);
	fclose(fd);
	int f = file_open(dest,'w');
	file_write(f,file);
	file_close(f);
	return 0;
}


int file_export(char* src, char* dest){
	FILE *fd = fopen(dest, "w");
	struct inode *file;    
	char *string;	 

	if(!find_file(src)){
		printf("No file named %s\n.",src);
		return;
    }

    file = find_file(src);
	printf("filesize: %d\n",file->size);
	string = (char*)malloc(sizeof(char)*file->size);
    lseek(fs.fd, file->blocks[0], SEEK_SET);
	int x = read(fs.fd, string, file->size);
	if(x == -1){
		printf("Read from fs failed.");
		return -1;
	}
	fprintf(fd,"%s", string);
	return 0;
}
