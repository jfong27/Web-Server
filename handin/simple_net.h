#ifndef NETH
#define NETH

#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

int create_service(in_port_t port);
int accept_connection(int fd);
#endif
