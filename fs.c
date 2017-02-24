/* Joe Teague and Clarence Jackson
 * CS560 Lab 1: The File System
 * fs.c - contains the actual functions for the filesystem. This one is gonna
 * be loooooong
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "fs.h"

#define FSNAME "fs_container"
#define FSFLAGS S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IWOTH

struct fs fs;

/* Some macros that might be useful within this file, but probably shouldn't
 * be exposed elsewhere. */
#define NODES_START ((struct inode *)fs.disk)
#define NODES_END   ((struct inode *)(fs.disk + DATA_START))
#define BLOCKS_START ((struct data_block *)(fs.disk + DATA_START))
#define BLOCKS_END \
    ((struct data_block *)(fs.disk + DISK_SIZE - LEFTOVER_DATA))
#define GET_INODE_OFFSET(n) \
    ((int)((void *)n - (void *)fs.disk))
#define GET_INODE_INDEX(n) \
    ((int)GET_INODE_OFFSET(n) / (sizeof(struct inode)))
#define GET_INODE_ADDR(i) \
    ((struct inode *)NODES_START + (struct inode*)(i * sizeof(struct inode)))
#define GET_BLOCK_ABS_OFFSET(b) \
    ((int)((void *)b - (void *)fs.disk))
#define GET_BLOCK_OFFSET(b) \
    ((int)GET_BLOCK_ABS_OFFSET(b) - DATA_START)
#define GET_BLOCK_INDEX(b) \
     ((int)GET_BLOCK_OFFSET(b) / (sizeof(struct data_block)))
#define GET_BLOCK_ADDR(o) \
    ((unsigned long)(fs.disk + o))

/* Find the first free inode on the filesystem
 * Returns NULL if no free inode is found, else returns a pointer to the
 * addr of the first free inode in memory. */
struct inode *find_free_inode() {
    struct inode *node = NULL;
    struct inode *tmp;
   
    printf("disk addr is 0x%x; node_start is 0x%x\n", fs.disk, NODES_START); 
    for(tmp = NODES_START; tmp < NODES_END; tmp++) {
        if(tmp->mode == NODE_MODE_UNUSED) {
            node = tmp;
            break;
        }
    }
    return node;
}

/* Like the above, but now we're looking for a free data block instead of an
 * inode */
struct data_block *find_free_block() {
    struct data_block *blk = NULL;
    struct data_block *tmp;

    printf("disk addr is 0x%x; block start is 0x%x ", fs.disk, BLOCKS_START);
    printf("disk end is 0x%x\n", BLOCKS_END);

    for(tmp = BLOCKS_START; tmp < BLOCKS_END; tmp++) {
        if(tmp->flags == BLK_MODE_UNUSED) {
            blk = tmp;
            break;
        }
    }
    return blk;
}

struct dir_entry *find_free_dir_entry(struct dir_block *blk) {
    int i;
    struct dir_entry *entry = NULL;

    for(i = 0; i < DIR_BLOCK_ENTRIES; i++) {
        if(blk->entries[i].entry_type == NODE_MODE_UNUSED) {
            entry = &(blk->entries[i]);
            break;
        }
    }

    return entry;
}

int fs_init() {
    if(access(FSNAME, F_OK) != -1) {
        printf("file system exists: ");
        if((fs.fd = open(FSNAME, O_RDWR, FSFLAGS)) < 0) {
            printf("ERROR! %d\n", errno);
            return -1;
        }
        read(fs.fd, (void *)fs.disk, DISK_SIZE);
        printf("fd %d\n", fs.fd);
    }
    else {
        fs.fd = -1;
        fs.data_start = -1;
        fs.cur_dir = NULL;
        printf("fs: no fs. Run \"mkfs\" to create one\n");
    }

    return 0;
}

void fs_info() {
    int blocks = 0;
    int inodes = 0;
    int dirs   = 0;
    int ptrs   = 0;
    int data   = 0;
    struct inode *tmp_node;
    struct inode *first_free_node = NULL;
    struct data_block *tmp_blk = NULL;
    void *first_free_blk = NULL;
    struct inode *nodes_start = (struct inode *)fs.disk;
    struct data_block *tmp_dat_block = NULL;
    struct dir_block *tmp_dir_block = NULL;
    struct data_block *blocks_end = (struct data_block *)(fs.disk + DISK_SIZE - LEFTOVER_DATA);
    void *barrier = fs.disk + DATA_START;

    /* Print some standard information about the filesystem. */
    printf("disk size:       %d\n", (int)DISK_SIZE);
    printf("inode size:      %d\n", (int)sizeof(struct inode));
    printf("num inodes:      %d\n", (int)NUM_INODES);
    printf("data size:       %d\n", (int)DATA_SIZE);
    printf("data block size: %d\n", (int)BLOCK_SIZE);
    printf("data start:      %d\n", (int)DATA_START);
    printf("num data blocks: %d\n", (int)NUM_DATA_BLOCKS);
    printf("dir block flags: %d\n", 
            (int)((void *)&tmp_dir_block->flags - (void *)tmp_dir_block));
    printf("dat block flags: %d\n",
            (int)((void *)&tmp_dat_block->flags - (void *)tmp_dat_block));
    printf("leftover data:   %d\n\n", (int)LEFTOVER_DATA);

    printf("directory entry size:    %d\n", (int)sizeof(struct dir_entry));
    printf("entries per data block:  %d\n", (int)DIR_BLOCK_ENTRIES);
    printf("entries size per block:  %d\n\n", (int)TOTAL_DIR_ENTRY_SIZE);

    /* If there's no filesystem loaded, we don't have specific information */
    if(fs.fd == -1) {
        printf("No filesystem loaded. No specific info to display.\n\n");
        return;
    }

    /* Calculate usage stats */
    for(tmp_node = nodes_start; tmp_node < (struct inode *)barrier; tmp_node++) {
        if(tmp_node->mode != NODE_MODE_UNUSED) {
            ++inodes;
        }
    }
    for(tmp_blk = (struct data_block *)barrier; tmp_blk < blocks_end;
            tmp_blk++) {
        if(tmp_blk->flags == BLK_MODE_DIR) {
            ++blocks;
            ++dirs;
        }
    }

    printf("inodes in use:      %d\n", inodes);
    printf("data blocks in use: %d\n", blocks);
}

int fs_mkfs() {
    int i, j;
    struct inode *inode;
    struct dir_block *block;

    /* Create the file */
    printf("creating new file system... ");
    fs.cur_dir = NULL;
    if(fs.fd == -1) {
        if((fs.fd = open(FSNAME, O_RDWR | O_CREAT, FSFLAGS)) < 0) {
            printf("error!\n");
            return -1;
        }
    }
    printf("fd is %d ", fs.fd);

    /* Create the inodes */
    for(i = 0; i < TOTAL_INODE_SIZE; i += sizeof(struct inode)) {
        inode = (struct inode *)&(fs.disk[i]);
        inode->mode = NODE_MODE_UNUSED;
        inode->size = 0;
        for(j = 0; j < 12; j++) {
            inode->blocks[i] = -1;
        }
        inode->ptr_block = -1;
    }

    /* Create the data blocks */
    for(i = DATA_START; i < DISK_SIZE; i++) {
        fs.disk[i] = 0;
    }

    lseek(fs.fd, 0, SEEK_SET); 
    if(write(fs.fd, (void *)fs.disk, DISK_SIZE) != DISK_SIZE) {
        printf("error\n");
        return -3;
    }
    fsync(fs.fd);

    fs_mkdir("");

    printf("done! \n");

    return 0;
}

int fs_mkdir(char *name) {
    int i;
    struct inode *new_node;
    struct dir_block *new_block;
    struct dir_block *pt_block;
    struct dir_entry *free_entry;

    /* locate and obtain pointers to a free inode and data block */
    new_node = find_free_inode();
    if(!new_node) {
        printf("fs error (fs_mkdir): could not locate free inode\n");
        return -1;
    }
    printf("fs: found free inode 0x%x (offset 0x%x) ", (void *)new_node, 
            GET_INODE_OFFSET(new_node));
    printf("corresponding to index %d\n", GET_INODE_INDEX(new_node));

    new_block = (struct dir_block *)find_free_block();
    if(!new_block) {
        printf("fs error (fs_mkdir): could not locate free data block.\n");
        return -2;
    }
    printf("fs: found free block 0x%x (offset 0x%x) ", (void *)new_block,
            GET_BLOCK_ABS_OFFSET(new_block));
    printf("corresponding to index %d\n", GET_BLOCK_INDEX(new_block));

    /* Set up the dir, handling the special case where no current durectory
     * exists (ie, when a new fs is created) by setting the parent of the
     * directory to itseld; this creates the root directory. */
    new_node->mode = NODE_MODE_DIR;
    new_node->size = 1;
    new_node->blocks[0] = GET_BLOCK_ABS_OFFSET(new_block);
    
    for(i = 0; i < DIR_BLOCK_ENTRIES; i++) {
        new_block->entries[i].entry_type = NODE_MODE_UNUSED;
        new_block->entries[i].entry_node = -1;
    }
    strcpy(new_block->entries[0].name, ".");
    new_block->entries[0].entry_type = NODE_MODE_DIR;
    new_block->entries[0].entry_node = GET_INODE_OFFSET(new_node);
    strcpy(new_block->entries[1].name, "..");
    new_block->entries[1].entry_type = NODE_MODE_DIR;
    
    /* Handle the special case when this is the first directory created, e.g.
     * when mkfs is first run */
    if(fs.cur_dir == NULL) {
        fs.cur_dir = new_node;
    }
    else {
        printf("fs: checking for free dir entry in block addr 0x%x\n",
                GET_BLOCK_ADDR(fs.cur_dir->blocks[0]));
        pt_block = (struct dir_block *)GET_BLOCK_ADDR(fs.cur_dir->blocks[0]);
        free_entry = find_free_dir_entry(pt_block);
        printf("fs: found free dir entry 0x%x in block index %d\n",
                free_entry);
        strcpy(free_entry->name, name);
        free_entry->entry_type = NODE_MODE_DIR;
        free_entry->entry_node = GET_INODE_OFFSET(new_node);
    }

    new_block->entries[1].entry_node = GET_INODE_OFFSET(fs.cur_dir);
    new_block->flags = BLK_MODE_DIR;

    /* Write the new inode and block */
    lseek(fs.fd, GET_INODE_OFFSET(new_node), SEEK_SET);
    if(write(fs.fd, (void *)new_node,
                sizeof(struct inode)) != sizeof(struct inode)) {
        printf("fs error (fs_mkdir): could not write inode\n");
        return -3;
    }

    lseek(fs.fd, GET_BLOCK_ABS_OFFSET(new_block), SEEK_SET);
    if(write(fs.fd, (void *)new_block, sizeof(struct dir_block)) !=
                sizeof(struct dir_block)) {
        printf("fs error (fs_mkdir): could not write data block\n");
        return -4;
    }

    lseek(fs.fd, GET_BLOCK_ABS_OFFSET(pt_block), SEEK_SET);
    if(write(fs.fd, (void *)pt_block, sizeof(struct dir_block)) !=
            sizeof(struct dir_block)) {
        printf("fs error (fs_mkdir): could not update parent dir block\n");
        return -5;
    }

    lseek(fs.fd, 0, SEEK_SET);

    return 0;
}

void fs_close() {
    close(fs.fd);
    fs.fd = -1;
}
