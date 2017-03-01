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
#include "file.h"

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
#define GET_ASSET_ADDR(offset) \
    ((unsigned long)(fs.disk + offset))

/* Find the first free inode on the filesystem
 * Returns NULL if no free inode is found, else returns a pointer to the
 * addr of the first free inode in memory. */
struct inode *find_free_inode() {
    struct inode *node = NULL;
    struct inode *tmp;
   
    /*printf("disk addr is 0x%x; node_start is 0x%x\n", fs.disk, NODES_START);*/
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

    /*printf("disk addr is 0x%x; block start is 0x%x ", fs.disk, BLOCKS_START);*/
    /*printf("disk end is 0x%x\n", BLOCKS_END);*/

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
        if(tmp_blk->flags == BLK_MODE_FILE) {
            ++blocks;
            ++data;
        }
    }

    printf("inodes in use:      %d\n", inodes);
    printf("blocks in use:      %d\n", blocks);
    printf("-- dir blocks:      %d\n", dirs);
    printf("-- file blocks:     %d\n", data);
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
    /*printf("fd is %d ", fs.fd);*/

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

	/* Create file descriptors */
	for(i = 0; i < 1024; i++){
	struct descriptor *descript = (struct descriptor*) malloc(sizeof(struct descriptor));
    fs.files[i] = descript;
	fs.files[i]->mode = NODE_MODE_UNUSED;
    fs.files[i]->flag = -1;
    fs.files[i]->offset = -1;
    fs.files[i]->node_ptr = NULL;
	}
    fsync(fs.fd);

    fs_mkdir("");

    printf("done! \n");

    return 0;
}

int fs_mkdir(char *name) {
    int i, is_new_fs;
    struct inode *new_node;
    struct dir_block *new_block;
    struct dir_block *pt_block;
    struct dir_entry *free_entry;

    /* make sure the file doesn't currently exist (as a file or directory) */
    /* But only if we actually have a current directory */
    if(fs.cur_dir) {
        /*printf("checking files...\n");*/
        if(find_file(name)) {
            /*printf("fs error (fs_mkdir): name already in use for file.\n");*/
            return -6;
        }

        /*printf("checking dirs...\n");*/
        if(find_dir(name)) {
            /*printf("fs error (fs_mkdir): directory already exists.\n");*/
            return -7;
        }
    }
    /* locate and obtain pointers to a free inode and data block */
    new_node = find_free_inode();
    if(!new_node) {
        /*printf("fs error (fs_mkdir): could not locate free inode\n");*/
        return -1;
    }
    /*printf("fs: found free inode 0x%x (offset 0x%x) ", (void *)new_node, 
            GET_INODE_OFFSET(new_node));*/
    /*printf("corresponding to index %d\n", GET_INODE_INDEX(new_node));*/

    new_block = (struct dir_block *)find_free_block();
    if(!new_block) {
        printf("fs error (fs_mkdir): could not locate free data block.\n");
        return -2;
    }
    /*printf("fs: found free block 0x%x (offset 0x%x) ", (void *)new_block,
            GET_BLOCK_ABS_OFFSET(new_block));*/
    /*printf("corresponding to index %d\n", GET_BLOCK_INDEX(new_block));*/

    /* Set up the dir, handling the special case where no current durectory
     * exists (ie, when a new fs is created) by setting the parent of the
     * directory to itseld; this creates the root directory. */
    new_node->mode = NODE_MODE_DIR;
    new_node->size = 0;
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
        is_new_fs = 1;
        fs.cur_dir_name = "";
    }
    else {
        /*printf("fs: checking for free dir entry in block addr 0x%x\n",
                GET_BLOCK_ADDR(fs.cur_dir->blocks[0]));*/
        pt_block = (struct dir_block *)GET_BLOCK_ADDR(fs.cur_dir->blocks[0]);
        fs.cur_dir->size++;
        free_entry = find_free_dir_entry(pt_block);
        /*printf("fs: found free dir entry 0x%x in block index %d\n",
                free_entry);*/
        strcpy(free_entry->name, name);
        free_entry->entry_type = NODE_MODE_DIR;
        free_entry->entry_node = GET_INODE_OFFSET(new_node);
        is_new_fs = 0;
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

    if(!is_new_fs) {
        lseek(fs.fd, GET_BLOCK_ABS_OFFSET(pt_block), SEEK_SET);
        if(write(fs.fd, (void *)pt_block, sizeof(struct dir_block)) !=
                sizeof(struct dir_block)) {
            printf("fs error (fs_mkdir): could not update parent dir block\n");
            return -5;
        }
    }

    lseek(fs.fd, 0, SEEK_SET);

    return 0;
}

/* Remove a directory named "name." Much like in Linux, non-empty directories
 * cannot be deleted using this command */
int fs_rmdir(char *name) {
    int i;
    struct inode *node;
    struct dir_block *blk;
    struct dir_entry *ent;

    /* Do not allow deletion of . or .. */
    if(strcmp(name, ".") == 0) {
        return -1;
    }
    else if(strcmp(name, "..") == 0) {
        return -2;
    }

    /* Make sure the name is in use and isn't a file */
    if(find_file(name)) {
        return -3;
    }

    node = find_dir(name);
    if(!node) {
        return -4;
    }

    blk = (struct dir_block *)GET_ASSET_ADDR(node->blocks[0]);

    /* Make sure we're not trying to delete the root directory */
    /* Root should be only dir where offset of . == offset of .. */
    if(blk->entries[0].entry_node == blk->entries[1].entry_node) {
        return -5;
    }

    /* Don't delete empty directories! */
    if(node->size != 0) {
        return -6;
    }

    /* Invalidate the inode and dir block */
    node->mode = NODE_MODE_UNUSED;
    node->size = 0;
    blk->flags = BLK_MODE_UNUSED;

    /* Find and delete the directory entry for the requested name */
    blk = (struct dir_block *)GET_ASSET_ADDR(fs.cur_dir->blocks[0]);
    for(i = 0; i < DIR_BLOCK_ENTRIES; i++) {
        ent = &blk->entries[i];
        if(strcmp(ent->name, name) == 0) {
            break;
        }
    }
    strcpy(ent->name, "");
    ent->entry_type = NODE_MODE_UNUSED;
    ent->entry_node = -1;
    fs.cur_dir->size--;

    return 0;
}

/* fs_ls: list all files in the current directory */
int fs_ls() {
    int i;
    struct dir_block *blk;
    struct dir_entry *ent;
    struct inode *node;

    /* Just in case there's no file system, bail out */
    if(fs.cur_dir == NULL) {
        printf("fs error (fs_ls): no filesystem exists.\n");
        return -1;
    }

    /* Print header */
    printf("Contents of ");
    if(strcmp(fs.cur_dir_name, "") == 0) {
        printf("root directory:\n");
    }
    else {
        printf("%s\n", fs.cur_dir_name);
    }

    /* Print directory information */
    printf("%-30s%10s%10s\n", "NAME", "TYPE", "SIZE");
    blk = (struct dir_block *)GET_BLOCK_ADDR(fs.cur_dir->blocks[0]);
    for(i = 0; i < DIR_BLOCK_ENTRIES; i++) {
        ent = &blk->entries[i];
        if(ent->entry_type == NODE_MODE_UNUSED) continue;
        printf("%-30s", ent->name);
        /* Get the object's inode to print type and size info */
        node = (struct inode *)GET_ASSET_ADDR(ent->entry_node);
        if(node->mode == NODE_MODE_DIR) {
            printf("%10s", "DIR");
        }
        else if(node->mode == NODE_MODE_FILE) {
            printf("%10s", "FILE");
        }
        printf("%10d\n", node->size);
    }

    return 0;
}

/* A function to recursively print a tree. May not be EXACTLY like Windows,
 * but it's pretty close */
/* To avoid recursion issues, we're not going to deal with . and .. here */
void fs_tree(struct inode *dir, int indent) {
    int i, j;
    struct inode *next;
    struct dir_block *blk;
    struct dir_entry *ent;

    /* Get the directory block of the current dir */
    if(indent == 0) {
        printf("\n.\n");
    }
    blk = (struct dir_block *)GET_ASSET_ADDR(dir->blocks[0]);
    for(i = 0; i < DIR_BLOCK_ENTRIES; i++) {
        ent = &blk->entries[i];
        if(ent->entry_type == NODE_MODE_UNUSED) {
            continue; 
        }
        if(strcmp(ent->name, ".") == 0 || strcmp(ent->name, "..") == 0) {
            continue;
        }

        for(j = 0; j < indent; j++) {
            printf("    ");
        }
        printf("|-- %s\n", ent->name);
        if(ent->entry_type == NODE_MODE_DIR) {
            next = (struct inode *)GET_ASSET_ADDR(ent->entry_node);
            fs_tree(next, indent + 1);
        }
    }
}

/* Change directories */
/* The simple case for changing to root */
void fs_cd_root() {
    fs.cur_dir = (struct inode *)GET_ASSET_ADDR(0);
    fs.cur_dir_name = "";
}

int fs_cd(char *name) {
    int i, j;
    int found = 0;
    int found_file = 0;
    struct inode *pt;               /* A "parent" inode for some ops */
    struct dir_block *blk, *blk2;
    struct dir_entry *ent, *ent2;

    /* Just in case there's no filesystem, bail out */
    if(fs.cur_dir == NULL) {
        return -1;
    }

    /* Get the directory block of the current directory */
    blk = (struct dir_block *)GET_BLOCK_ADDR(fs.cur_dir->blocks[0]);
    for(i = 0; i < DIR_BLOCK_ENTRIES; i++) {
        ent = &blk->entries[i];
        if(strcmp(name, ent->name) == 0 && ent->entry_type == NODE_MODE_DIR) {
            fs.cur_dir = (struct inode *)GET_ASSET_ADDR(ent->entry_node);
            /* Handle the special cases of switching to "." or ".." so the name
             * displays correctly when ls is called */
            if(strcmp(name, ".") != 0) {
                fs.cur_dir_name = ent->name;
            }
            if(strcmp(name, "..") == 0) {
                /* This seems awfully complex for what it is. But, if names
                 * aren't stored in the inodes, it's the only way. */
                blk2 = (struct dir_block *)GET_BLOCK_ADDR(fs.cur_dir->blocks[0]);
                pt = (struct inode *)GET_ASSET_ADDR(blk2->entries[1].entry_node);
                blk2 = (struct dir_block *)GET_BLOCK_ADDR(pt->blocks[0]);
                for(j = 0; j < DIR_BLOCK_ENTRIES; j++) {
                    /* Really? We're still going? */
                    ent2 = &blk2->entries[j];
                    if(ent2->entry_node == ent->entry_node) {
                        fs.cur_dir_name = ent2->name;
                        break;
                    }
                }
            }
            
            /* Handle the special case of switching back to the root
             * directory, since it has no actual name. */
            blk2 = (struct dir_block *)GET_BLOCK_ADDR(fs.cur_dir->blocks[0]);
            if(blk2->entries[0].entry_node == blk2->entries[1].entry_node) {
                fs.cur_dir_name = "";
            }
            found = 1;
            break;
        }
        else if(strcmp(name, ent->name) == 0) {
            found_file = 1;
            break;
        }
    }

    if(found_file && !found) {
        return -2;
    }
    else if(!found_file && !found) {
        return -3;
    }

    return 0;
}

void fs_close() {
    close(fs.fd);
    fs.fd = -1;
}

/* File operation functions. Most of the functions that actually use these
 * exist within file.c, but since there are file system operations I thought it
 * would be prudent to put them here. */

/* Create a file within the current directory */
struct inode *create_file(char *name) {
    int i;
    struct inode *new_node = NULL;
    struct data_block *new_blk = NULL;
    struct dir_block *cur;
    struct dir_entry *ent;

    /* Make sure the filename isn't already in use */
    if(find_file(name)) {
        printf("fs error (create_file): file already exists.\n");
        return NULL;
    }

    if(find_dir(name)) {
        printf("fs error (create_file): name already in use by directory.\n");
        return NULL;
    }

    /* Get an inode and data block for the new file */
    new_node = find_free_inode();
    if(!new_node) {
        printf("fs error (create_file): could not find free inode\n");
        return NULL;
    }
    
    new_blk = find_free_block();
    if(!new_blk) {
        printf("fs error (create_file): could not find free data block\n");
        return NULL;
    }
    
    /* Get a free directory entry slot in the block */
    cur = (struct dir_block *)GET_BLOCK_ADDR(fs.cur_dir->blocks[0]);
    for(i = 0; i < DIR_BLOCK_ENTRIES; i++) {
        ent = &cur->entries[i];
        if(ent->entry_type == NODE_MODE_UNUSED) {
            break;
        }
    }
    
    if(i == DIR_BLOCK_ENTRIES) {
        printf("fs error (create_file): could not find free entry slot\n");
        return NULL;
    }

    /* Configure the NEW data structures */
    new_node->mode = NODE_MODE_FILE;
    new_node->size = 0;
    new_node->blocks[0] = GET_BLOCK_ABS_OFFSET(new_blk);
    for(i = 0; i < sizeof(struct data_block) - sizeof(int); i++) {
        new_blk->data[i] = '\0';
    }
    new_blk->flags = BLK_MODE_FILE;

    /* Update the current directory to include the new file */
    strcpy(ent->name, name);
    ent->entry_type = NODE_MODE_FILE;
    ent->entry_node = GET_INODE_OFFSET(new_node);
    fs.cur_dir->size++;

    return new_node;
}

/* Delete a file */
int delete_file(char *name) {
    int i;
    int found = 0;
    int found_dir = 0;
    struct inode *file_node = NULL;
    struct data_block *file_block;
    struct dir_block *cur;
    struct dir_entry *ent;

    /* Make sure we're not trying to delete a directory */
    if(find_dir(name)) {
        return -1;
    }

    /* Get a pointer to the file */
    file_node = find_file(name);
    /*printf("file: 0x%x\n", file_node);*/
    if(!file_node) {
        return -2;
    }
    
    /* Get a reference to the directory */
    cur = (struct dir_block *)GET_ASSET_ADDR((fs.cur_dir->blocks[0]));
    for(i = 0; i < DIR_BLOCK_ENTRIES; i++) {
        ent = &cur->entries[i];
        if(strcmp(ent->name, name) == 0) {
            break;
        }
    }
    /* Get pointers to the data block and inode */
    file_node = (struct inode *)GET_ASSET_ADDR(ent->entry_node);
    file_block = (struct data_block *)GET_ASSET_ADDR(file_node->blocks[0]);

    /* Invalidate the entry, inode, and data block */
    strcpy(ent->name, "");
    ent->entry_type = NODE_MODE_UNUSED;
    ent->entry_node = -1;

    file_node->mode = NODE_MODE_UNUSED;
    file_node->size = 0;
    file_node->blocks[0] = -1;

    file_block->flags = BLK_MODE_UNUSED;

    fs.cur_dir->size--;

    return 0;
}

/* Find a file in the current directory and return its inode */
struct inode *find_file(char *name) {
    int i;
    int found = 0;
    int found_dir = 0;

    struct inode *file_node = NULL;
    struct dir_block *cur;
    struct dir_entry *ent;

    cur = (struct dir_block *)GET_BLOCK_ADDR(fs.cur_dir->blocks[0]);
    for(i = 0; i < DIR_BLOCK_ENTRIES; i++) {
        ent = &cur->entries[i];
        if(strcmp(ent->name, name) == 0 && ent->entry_type == NODE_MODE_FILE) {
            found = 1;
            file_node = (struct inode *)GET_ASSET_ADDR(ent->entry_node);
            break;
        }
        else if(strcmp(ent->name, name) == 0) {
            found_dir = 1;
            break;
        }
    }
   
    return file_node;
}

/* Find a directory, too. This might be useful. */
struct inode *find_dir(char *name) {
    int i;
    int found = 0;
    int found_file = 0;

    struct inode *dir_node = NULL;
    struct dir_block *cur;
    struct dir_entry *ent;

    cur = (struct dir_block *)GET_BLOCK_ADDR(fs.cur_dir->blocks[0]);
    for(i = 0; i < DIR_BLOCK_ENTRIES; i++) {
        ent = &cur->entries[i];
        if(strcmp(ent->name, name) == 0 && ent->entry_type == NODE_MODE_DIR) {
            found = 1;
            dir_node = (struct inode *)GET_ASSET_ADDR(ent->entry_node);
            break;
        }
        else if(strcmp(ent->name, name) == 0) {
            found_file = 1;
            break;
        }
    }
   
    return dir_node;
}
