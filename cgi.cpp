#include "cgi.h"

int cgi_conn::m_epollfd = -1;

cgi_conn::cgi_conn()
{
}

cgi_conn::~cgi_conn()
{

}

int cgi_conn::sockfd()
{
    return m_sockfd;
}

void cgi_conn::init(int sockfd, int epollfd = -1)
{
    m_epollfd = epollfd;
    m_sockfd = sockfd;
    memset(m_buf, 0, sizeof(m_buf));
    m_read_idx = 0;
    m_status = STATUS_FREE;
}

void cgi_conn::process()
{
    int idx = 0;
    int ret = -1;
    m_read_idx = 0;
    m_status = STATUS_WORKING;
    memset(m_buf, 0, sizeof(m_buf));
    while (true)
    {
        idx = m_read_idx;
        ret = recv(m_sockfd, m_buf+idx, BUFFER_SIZE-idx-1, 0);
        if (ret < 0)
        {
            if (errno != EAGAIN)
            {
                printf("Client closed dected, fd:%d\n", m_sockfd);
                removefd(m_epollfd, m_sockfd);
                m_status = STATUS_CLOSED;
            }
            break;
        }
        else if (ret == 0)
        {
            printf("Client closed dected 2, fd:%d\n", m_sockfd);
            removefd(m_epollfd, m_sockfd);
            shutdown(m_sockfd, 0);
            m_status = STATUS_CLOSED;
            break;
        }
        else
        {
            m_read_idx += ret;
            printf("user input is %s\n", m_buf);
            for ( ; idx<m_read_idx; ++idx)
            {
                if((idx>=1) && (m_buf[idx-1] == '\r') && (m_buf[idx] == '\n'))
                {
                    break;
                }
            }

            if (idx == m_read_idx)
            {
                continue;
            }
            m_buf[idx-1] = '\0';

            char *filename = m_buf;
            if (access(filename, F_OK) == -1)
            {
                printf("filename %s does not exist!\n", m_buf);
                removefd(m_epollfd, m_sockfd);
                m_status = STATUS_CLOSED;
                break;
            }

            ret = fork();
            if (ret == -1)
            {
                removefd(m_epollfd, m_sockfd);
                m_status = STATUS_CLOSED;
                break;
            }
            else if(ret > 0)
            {
                // removefd(m_epollfd, m_sockfd);
                m_status = STATUS_FREE;
                break;
            }
            else
            {
                close(STDOUT_FILENO);
                dup(m_sockfd);
                close(m_sockfd);
                execl(m_buf, m_buf, 0);
                removefd(m_epollfd, STDOUT_FILENO);
                printf("cgi_conn exited!\n");
                exit(0);
            }
        }
    }
}

bool cgi_conn::isFree()
{
    return (m_status == STATUS_FREE);
}
bool cgi_conn::isWorking()
{
    return (m_status == STATUS_WORKING);
}
bool cgi_conn::isClosed()
{
    return (m_status == STATUS_CLOSED);
}

int main(int argc, char *argv[])
{
    if (argc <= 3)
    {
        printf("Usage: %s ip_address port process_num\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int pnum = atoi(argv[3]);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    int ret = 0;
    int flag = 1;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    ret = listen(listenfd, 5);
    assert(ret != -1);
#ifdef EPOLL
    processpool *pool = ppool_epoll< cgi_conn >::create(listenfd, pnum);
#else
    processpool *pool = ppool_select< cgi_conn >::create(listenfd, pnum);
#endif
    if (pool)
    {
        pool->run();
        printf("Pool finished!\n");
        delete pool;
    }

    close(listenfd);
    return 0;
}

