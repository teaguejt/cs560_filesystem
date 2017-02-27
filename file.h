/* Joseph Teague and Clarence Jackson
 * CS560 Lab 1: The File System
 * file.h: Structures and Definitions for file operations. 
 */


#define FSNAME "fs_container"

/* descriptor Modes */
#define NODE_MODE_UNUSED 0
#define NODE_MODE_FILE   1

/*file name max*/
#define MAX_FILE_LENGTH 255

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
char *file_read(int fd, int size);
int file_write(int fd, char *string);
struct descriptor *get_fd(int index);
int file_open(char *name, char flag);
