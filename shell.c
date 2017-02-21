/* Joe Teague and Clarence Jackson
 * CS560 Lab 1 - filesystem
 * shell.c - contains the command and string parsing needed for users to
 * actually interface with the filesystem
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"

void listen() {
    char *line = NULL;
    size_t length;
    int status;
    int proceed = 1;

    do {
        status = getline(&line, &length, stdin);
        if(status == -1) {
            printf("fs: input error\n");
            continue;
        }
        line[strlen(line) - 1] = '\0';   /* "Strip" newline */

        printf("%s\n", line);

        if(strcmp(line, "exit") == 0) {
            proceed = 0;
        }
    } while(proceed);

    return;
}
