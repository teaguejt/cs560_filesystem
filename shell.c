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

int shell_valid_string(char *str) {
    int i;

    /* Check to make sure there are no spaces in the string */
    for(i = 0; i < strlen(str); i++) {
        if(str[i] == ' ') {
            return -1;
        }
    }

    /* Check to make sure the string is less than 255 long */
    if(strlen(str) > 255) {
        return -2;
    }

    return 0;
}

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
        part_break = i + 1; /* There will be a space here, so ignore it. */

        /* Now handle the multitude of commands we can have here. */
        if(strcmp(cmd, "mkfs") == 0) {
            printf("cmd: %s -- make a file system\n");
            fs_mkfs();
        }
        else if(strcmp(cmd, "info") == 0) {
            fs_info();
        }
        else if(strcmp(cmd, "mkdir") == 0) {
            if(shell_valid_string(&line[part_break])) {
                printf("fs error: invalid string to mkdir\n");
                continue;
            }
            
            printf("fs: will make directory %s\n", &line[part_break]);
            fs_mkdir(&line[part_break]);
        }
        else if(strcmp(cmd, "ls") == 0) {
            fs_ls();
        }
        else if(strcmp(cmd, "cd") == 0) {
            if(shell_valid_string(&line[part_break])) {
                printf("fs error: invalid string to mkdir\n");
                continue;
            }

            fs_cd(&line[part_break]);
        }
        else if(strcmp(cmd, "exit") == 0) {
            printf("fs: goodbye\n");
            proceed = 0;
            break;
        }
        else {
            printf("invalid cmd %s %s\n", cmd, &line[part_break]);
        }

    } while(proceed);

    return;
}
