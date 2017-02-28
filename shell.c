/* Joe Teague and Clarence Jackson
 * CS560 Lab 1 - filesystem
 * shell.c - contains the command and string parsing needed for users to
 * actually interface with the filesystem
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"
#include "file.h"
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

/* This one is a little funky. We want to take a string for a path, break it 
 * at the slashes by replacing them with '\0', then return the number of parts
 * it was broken into. The receiving function can then use this number and
 * strlen to iterate over the path components. */
int process_path_string(char *path) {
    int parts = 1;
    int i, len;

    len = strlen(path);
    for(i = 0; i < len; i++) {
        if(path[i] == '/' && i != len - 1) {
            ++parts;
            path[i] = '\0';
        }
        else if(path[i] == '/') {
            path[i] = '\0';
        }
    }

    return parts;
}

char *get_last_path_part(int count, char *path) {
    int i;
    char *rv = path;
    
    for(i = 0; i < count - 1; i++) {
        rv += strlen(rv) + 1;
    }

    return rv;
}


void mass_change(int count, char *path) {
    int i;
    int cdrv;
    for(i = 0; i < count; i++) {
        /* Don't make directories called "" */
        if(strcmp("", path)) {
            cdrv = fs_cd(path);
            if(cdrv) {
                printf("fs error: cd returned %d\n", cdrv);
                break;
            }
            path += strlen(path) + 1;
        }
    }
}

void listen() {
    char *part;
    char *line = NULL;
    char cmd[CMD_LEN];
    size_t length;
    int i, status, part_break, rv, count, modifier;
    int proceed = 1;
    struct inode *tmp_node;
    char *tmp_name;

    do {
        /* Reset the buffer for the first part of the command. */
        for(i = 0; i < CMD_LEN; i++) {
            cmd[i] = '\0';
        }

        /* Read a line from stdin and break on error or EOF */
        printf("\njorance$ ");
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

            tmp_node = fs.cur_dir;
            tmp_name = fs.cur_dir_name;

            if(line[part_break] == '/') {
                modifier = 1;
                fs_cd_root();
            }
            else {
                modifier = 0;
            }
            count = process_path_string(&line[part_break + modifier]);
            mass_change(count - 1, &line[part_break + modifier]);
            fs_mkdir(get_last_path_part(count, &line[part_break + modifier]));

            fs.cur_dir = tmp_node;
            fs.cur_dir_name = tmp_name;
        }
        else if(strcmp(cmd, "ls") == 0) {
            fs_ls();
        }
        else if(strcmp(cmd, "cd") == 0) {
            if(shell_valid_string(&line[part_break])) {
                printf("fs error: invalid string to mkdir\n");
                continue;
            }
            if(line[part_break] == '/') {
                modifier = 1;
                fs_cd_root();
            }
            else {
                modifier = 0;
            }
            count = process_path_string(&line[part_break + modifier]);

            mass_change(count, &line[part_break + modifier]);
        }
        else if(strcmp(cmd, "touch") == 0) {
            if(shell_valid_string(&line[part_break])) {
                printf("fs error: invalid string to touch\n");
                continue;
            }
            
            tmp_node = fs.cur_dir;
            tmp_name = fs.cur_dir_name;

            if(line[part_break] == '/') {
                modifier = 1;
                fs_cd_root();
            }
            else {
                modifier = 0;
            }
            count = process_path_string(&line[part_break + modifier]);
            mass_change(count - 1, &line[part_break + modifier]);
            
            rv = (long)create_file(get_last_path_part(count,
                        &line[part_break + modifier]));

            if(!rv) {
                printf("fs error: create_file returned NULL!\n");
            }

            fs.cur_dir = tmp_node;
            fs.cur_dir_name = tmp_name;

        }
        else if(strcmp(cmd, "rm") == 0) {
            if(shell_valid_string(&line[part_break])) {
                printf("fs error: invalid string to rm\n");
                continue;
            }
            
            tmp_node = fs.cur_dir;
            tmp_name = fs.cur_dir_name;

            if(line[part_break] == '/') {
                modifier = 1;
                fs_cd_root();
            }
            else {
                modifier = 0;
            }
            count = process_path_string(&line[part_break + modifier]);
            mass_change(count - 1, &line[part_break + modifier]);
            
            rv = delete_file(get_last_path_part(count,
                        &line[part_break + modifier]));

            if(rv == -1) {
                printf("fs error: %s is a directory\n", &line[part_break]);
            }
            else if(rv == -2) {
                printf("fs error: %s does not exist.\n", &line[part_break]);
            }

            fs.cur_dir = tmp_node;
            fs.cur_dir_name = tmp_name;
        }
        else if(strcmp(cmd, "rmdir") == 0) {
            if(shell_valid_string(&line[part_break])) {
                printf("fs error: invalid string to rm\n");
                continue;
            }
            
            tmp_node = fs.cur_dir;
            tmp_name = fs.cur_dir_name;

            if(line[part_break] == '/') {
                modifier = 1;
                fs_cd_root();
            }
            else {
                modifier = 0;
            }
            count = process_path_string(&line[part_break + modifier]);
            mass_change(count - 1, &line[part_break + modifier]);
            
            rv = fs_rmdir(get_last_path_part(count,
                        &line[part_break + modifier]));

            if(rv) {
                printf("fs error: rmdir returned %d\n", rv);
            }

            fs.cur_dir = tmp_node;
            fs.cur_dir_name = tmp_name;
        }
        else if(strcmp(cmd, "tree") == 0) {
            fs_tree(fs.cur_dir, 0);
        }
        else if(strcmp(cmd, "parse") == 0) {
            if(shell_valid_string(&line[part_break])) {
                printf("fs error: invalid string\n");
                continue;
            }
            
            if(line[part_break] == '/') {
                count = process_path_string(&line[part_break + 1]);
                part = &line[part_break + 1];
            }
            else {
                count = process_path_string(&line[part_break]);
                part = &line[part_break];
            }

            for(i = 0; i < count; i++) {
                printf("%3d: %s\n", i, part);
                part = part + strlen(part) + 1;
            }
        }
        /*finalize file operation parsing*/
		else if(strcmp(cmd, "open") == 0) {
			int p = 0;
			int s = 0;
			char* file = (char*) malloc(sizeof(char)*256);
			char mode = '\0';
			
			if(line[part_break] == '\0'){
				printf("fs error: invalid input to open.\n");
				continue;
			}

			while(line[part_break + p] != '\0') {
				if (line[part_break + p] == ' '){
					++s;
					++p;
					continue;
				}
				if (s == 0)	
					file[p] = line[part_break + p];
				if (s == 1)
					mode = line[part_break + p];
				++p;
				if(p > 256 || s > 1 || (mode && (mode != 'r' && mode != 'w')) || (mode && line[part_break + p]) || (line[part_break + p+1] == '\0' && s == 0)){
					printf("fs error: invalid input to open.\n");
					p = -1;
					break;
				}
			}
			
			if (p < 0){
				continue;
			}
		  	else{
				file_open(file,mode);
			}
        }
		else if(strcmp(cmd, "read") == 0) {
			int p = 0;
			int x = 0;
			int s = 0;
			char* w;
			char d[5];
			char n[10];
			int sz = 0;
			int fd = 0;
			
			if(line[part_break] == '\0'){
				printf("fs error: invalid input to read.\n");
				continue;
			}

			while(line[part_break + p] != '\0') {
				if (line[part_break + p] == ' '){
					++s;
					++p;
					continue;
				}

				if (s == 0)	
					d[p] = line[part_break + p];
  
				if (s == 1){
					n[x] = line[part_break + p];
					++x;
				}
				++p;
				if(p > 12 || s > 1 || (p > 4 && s == 0) || (line[part_break + p+1] == '\0' && s == 0)){
					printf("fs error: invalid input to read.\n");
					p = -1;
					break;
				}
			}
			if (p <= 0){
				continue;
			}
			else{
				fd = strtol(d, (char **)NULL, 10);
				sz = strtol(n, (char **)NULL, 10);
				if(fd > 1024 || sz > 1000000){	
					printf("fs error: invalid input to read.\n");
					continue;
				}
				w = file_read(fd,sz);
				if(w)
					printf("%s\n",w);
			}
        }
		else if(strcmp(cmd, "write") == 0) {
		    file_write(1,"Hello World\n This is the new me\n How are you?");
		    file_seek(1,0);
        }
		else if(strcmp(cmd, "seek") == 0) {
		    file_seek(0,6);
		    char *x = file_read(0,5);
			if(x)
				printf("%s\n",x);
		    file_seek(1,11);
			file_write(1,"\n This is the old me\n Who are you?");
		    file_seek(0,0);
		    file_seek(1,0);
        }
		else if(strcmp(cmd, "close") == 0) {
		    file_close(0);
		    file_close(1);
        }
		else if(strcmp(cmd, "cat") == 0) {
		    file_cat("filea");
        }
		else if(strcmp(cmd, "import") == 0) {
		    file_import("test.txt","test.txt");
        }
		else if(strcmp(cmd, "export") == 0) {
		    file_export("filea","filea.txt");
        }
        /*end of file operation parsing*/
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
