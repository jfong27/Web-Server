#ifndef HTTPD_H
#define HTTPD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#define START_SIZE 100
#define HEAD 5
#define GET 6
#define CHILD 1
#define PIPE 2

void handle_request(int fd);
void child_handler();
void pipe_handler();
void setup_handlers();
char *read_request(int fd);
char *proc_req(char *req);
char *process_get(char *file);
char *cgi(char *file, int req);
char *exec_cgi(char **cmd,int req);
char *check_exit(int status);
char *get_header(char *file);
char *check_perm(char *file);
char *get_contents(char *file);
char *err_response(char *type);
char *add_dot_slash(char *file);
void *checked_malloc(size_t size);



#endif 
