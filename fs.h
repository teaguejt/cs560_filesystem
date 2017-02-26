/* Joseph Teague and Clarence Jackson
 * CS560 Lab 1: The File System
 * fs.h: contains structures and definitions related to the file system
 * container.
 */

/* Constants */
/* General */
#define DISK_SIZE  (100<<20)
#define BLOCK_SIZE (64<<10)    /* The size of data blocks in the fs */
#define NUM_INODES ((int)DISK_SIZE / (int)BLOCK_SIZE)
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

/* A basic data block */
struct data_block {
    char data[BLOCK_SIZE - sizeof(int)];
    int flags;
};

/* A directory entry and directory data block */
struct dir_entry {
    char name[256];     /* Filename */
    int entry_type;     /* Entry type, from inode type above */
    int entry_node;     /* The entry's inode index */
};

#define DIR_BLOCK_ENTRIES ((int)BLOCK_SIZE / sizeof(struct dir_entry))
#define TOTAL_DIR_ENTRY_SIZE (sizeof(struct dir_entry) * DIR_BLOCK_ENTRIES)


struct dir_block {
    struct dir_entry entries[DIR_BLOCK_ENTRIES];
    char pad[BLOCK_SIZE - TOTAL_DIR_ENTRY_SIZE - sizeof(int)];
    int flags;
};

/* A structure for a data block of pointers to data blocks */


/* The inode structure */
struct inode {
    int mode;       /* inode mode, from above */
    int size;       /* size of the file */
    int blocks[12]; /* direct data blocks */
    int ptr_block;  /* indirect data block */
};

#define TOTAL_INODE_SIZE (sizeof(struct inode) * NUM_INODES)

struct fs {
    int fd;
    int data_start;
    struct inode *cur_dir;
    char *cur_dir_name;
    char disk[DISK_SIZE];
};

#define DATA_START (int)TOTAL_INODE_SIZE
#define DATA_SIZE ((int)DISK_SIZE - (int)TOTAL_INODE_SIZE)
#define NUM_DATA_BLOCKS ((int)DATA_SIZE / (int)BLOCK_SIZE)
#define LEFTOVER_DATA ((int)DATA_SIZE % (int)BLOCK_SIZE)

/* I don't know if this needs to be extern, but for now I'll leave it */
extern struct fs fs;

/* Function prototypes */
int fs_init();
void fs_info();
int fs_mkfs();
int fs_mkdir(char *name);
int fs_ls();
int fs_rmdir(char *name);
void fs_close();
void fs_cd(char *name);
void fs_cd_root();

/* Some file operation functions */
struct inode *create_file(char *name);
int delete_file(char *name);
struct inode *find_file(char *name);
struct inode *find_dir(char *name);
