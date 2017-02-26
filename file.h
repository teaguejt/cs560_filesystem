/* Joseph Teague and Clarence Jackson
 * CS560 Lab 1: The File System
 * fs.h: contains structures and definitions related to the file system
 * container.
 */

/* descriptor Modes */
#define NODE_MODE_UNUSED 0
#define NODE_MODE_FILE   1

#include <stdlib.h>

//#include "fs.h" 

/* file descriptor */
struct descriptor {
      int mode;
	  char flag;
	  int offset;
	  struct inode *node_ptr;
};

/* Function prototypes */
int file_seek(int fd, int offset);
int file_close(int fd);
int file_read(int fd, int size);
int file_write(int fd, char *string);
struct descriptor *get_fd(int index);

