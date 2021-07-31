#include "cgi.h"

int cgi_conn::m_epollfd = -1;

cgi_conn::cgi_conn()
{

}

cgi_conn::~cgi_conn()
{

}

void cgi_conn::init(int epollfd, int sockfd, const sockaddr_in& client_addr)
{
    m_epollfd = epollfd;
    m_sockfd = sockfd;
    m_address = client_addr;
    memset(m_buf, 0, sizeof(m_buf));
    m_read_idx = 0;
}

void cgi_conn::process()
{
    int idx = 0;
    int ret = -1;
    while (true)
    {
        idx = m_read_idx;
        ret = recv(m_sockfd, m_buf+idx, BUFFER_SIZE-idx-1, 0);
        if (ret < 0)
        {
            if (errno != EAGAIN)
            {
                removefd(m_epollfd, m_sockfd);
            }
            break;
        }
        else if (ret == 0)
        {
            removefd(m_epollfd, m_sockfd);
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
                break;
            }

            ret = fork();
            if (ret == -1)
            {
                removefd(m_epollfd, m_sockfd);
                break;
            }
            else if(ret > 0)
            {
                removefd(m_epollfd, m_sockfd);
                break;
            }
            else
            {
                close(STDOUT_FILENO);
                dup(m_sockfd);
                execl(m_buf, m_buf, 0);
                exit(0);
            }
        }
    }
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
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    ret = listen(listenfd, 5);
    assert(ret != -1);
    processpool< cgi_conn > *pool = processpool< cgi_conn >::create(listenfd, pnum);
    if (pool)
    {
        pool->run();
        delete pool;
    }

    close(listenfd);
    return 0;
}