/* Joseph Teague and Clarence Jackson
 * CS560 Lab 1: The File System
 * fs.h: contains structures and definitions related to the file system
 * container.
 */

/* Constants */
/* General */
#define DISK_SIZE  (100<<20)
#define BLOCK_SIZE (4<<10)    /* The size of data blocks in the fs */
#define NUM_INODES (DISK_SIZE / BLOCK_SIZE)
/* inode Modes */
#define NODE_MODE_UNUSED 0
#define NODE_MODE_FILE   1
#define NODE_MODE_DIR    2
/* Data block modes */
#define BLK_MODE_UNUSED 0
#define BLK_MODE_FILE   1
#define BLK_MODE_DIR    2
#define BLK_MODE_PTRS   3


#include <stdlib.h>
/* A directory entry and data block */
struct dir_entry {
    char name[256];     /* Filename */
    int entry_type;     /* Entry type, from inode type above */
    int entry_node;     /* The entry's inode index */
};

struct dir_block {
    struct dir_entry entries[BLOCK_SIZE / sizeof(struct dir_entry)];
};

/* A structure for a data block of pointers to data blocks */


/* The inode structure */
struct inode {
    int mode;       /* inode mode, from above */
    int size;       /* size of the file */
    int blocks[12]; /* direct data blocks */
    int ptr_block;  /* indirect data block */
};

struct fs {
    FILE *f;
    int fd;
    struct inode inodes[NUM_INODES];
    char data[DISK_SIZE - (sizeof(struct inode) * NUM_INODES)];
};

extern struct fs *fs;

