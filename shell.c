/* Joe Teague and Clarence Jackson
 * CS560 Lab 1 - filesystem
 * shell.c - contains the command and string parsing needed for users to
 * actually interface with the filesystem
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"
#include "shell.h"

void listen() {
    char *line = NULL;
    char cmd[CMD_LEN];
    size_t length;
    int i, status, part_break;
    int proceed = 1;

    do {
        /* Reset the buffer for the first part of the command. */
        for(i = 0; i < CMD_LEN; i++) {
            cmd[i] = '\0';
        }

        /* Read a line from stdin and break on error or EOF */
        status = getline(&line, &length, stdin);
        if(status == -1) {
            break;
        }
        line[strlen(line) - 1] = '\0';   /* "Strip" newline */

        /* Isolate the first part of the command */
        i = 0;
        while(line[i] != ' ' && line[i] != '\0') {
            cmd[i] = line[i];
            ++i;
        }
        part_break = i + 1;

        /* Now handle the multitude of commands we can have here. */
        if(strcmp(cmd, "mkfs") == 0) {
            printf("cmd: %s -- make a file system\n");
            fs_mkfs();
        }
        else if(strcmp(cmd, "info") == 0) {
            fs_info();
        }
        else {
            printf("invalid cmd %s %s\n", cmd, &line[part_break]);
        }

        if(strcmp(line, "exit") == 0) {
            proceed = 0;
        }
    } while(proceed);

    return;
}
