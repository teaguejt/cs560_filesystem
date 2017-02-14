/* Joseph Teague and Clarence Jackson
 * CS560 Lab 1: The File System
 * fs.h: contains structures and definitions related to the file system
 * container.
 */

#include <stdlib.h>

struct fs {
    FILE *f;
    int fd;
};

extern struct fs *fs;
