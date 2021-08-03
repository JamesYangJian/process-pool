#ifndef _UTILS_H__
#define _UTILS_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define M_MAX(a,b) (a>b ? a : b)

extern int sig_pipefd[2];
void sig_handler( int sig);
void addsig(int sig, void(handler)(int), bool restart=true);
int setnonblocking( int fd);
void addfd(int epollfd, int fd);
void removefd(int epollfd, int fd);
void send_fd(int fd, int fd_to_send);
int recv_fd(int fd);

class process
{
public:
    process() : m_pid(-1) {}

    pid_t m_pid;
    int m_pipefd[2];
};


#endif