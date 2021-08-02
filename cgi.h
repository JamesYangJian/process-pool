#ifndef __CGI_H__
#define __CGI_H__

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

#ifdef EPOLL
#include "ppool_epoll.h"
#else
#include "ppool_select.h"
#endif

typedef enum
{
    STATUS_FREE = 0,
    STATUS_WORKING,
    STATUS_CLOSED
}CONN_STATUS;

class cgi_conn
{
public:
    cgi_conn();
    ~cgi_conn();
    void init(int sockfd, const sockaddr_in& client_addr, int epollfd);
    void process();
    int sockfd();
    bool isFree();
    bool isWorking();
    bool isClosed();

private:
    static const int BUFFER_SIZE = 1024;
    static int m_epollfd;
    int m_sockfd;
    sockaddr_in m_address;
    char m_buf[BUFFER_SIZE];
    int m_read_idx;
    CONN_STATUS m_status; 
};

#endif