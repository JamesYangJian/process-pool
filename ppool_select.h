#ifndef PPOOL_SELECT_H__
#define PPOOL_SELECT_H__
#include <netinet/in.h>

#include "ppool.h"
#include "chain.h"
#include "utils.h"

template< typename T>
class ppool_select : public processpool
{
private:
    ppool_select(int listenfd, int process_number=8):processpool(listenfd, process_number)
    {
        m_maxfd = 0;
        m_user_chain = NULL;
    }

public:
    static ppool_select<T> * create(int listenfd, int process_number=8)
    {
        if (m_instance == NULL)
        {
            m_instance = new ppool_select<T>(listenfd, process_number);
        }
        return m_instance;
    }

private:
    void prepare_multiplexing()
    {
        if (is_parent())
        {
            m_maxfd = M_MAX(m_maxfd, m_listenfd);
        }
        else
        {
            int pipefd = m_sub_process[m_idx].m_pipefd[1];
            m_maxfd = M_MAX(m_maxfd, pipefd);
            setnonblocking(m_listenfd);
        }
        m_maxfd = M_MAX(m_maxfd, sig_pipefd[0]);
    }

    void run_parent()
    {
        struct timeval tv;
        int iRet = -1;
        int new_conn = 1;

        setup_sig_pipe();
        prepare_multiplexing();

        while (!m_stop)
        {
            tv.tv_sec = 10;
            tv.tv_usec = 0;
            update_fdset();

            iRet = select(m_maxfd+1, &m_rdset, NULL, &m_exceptset, &tv);
            if (iRet < 0 )
            {
                if (errno != EINTR)
                {
                    printf("Parent select exception!\n");
                    break;
                }
                continue;
            }
            else if (iRet == 0)
            {
                continue;
            }
            else
            {
                if (FD_ISSET(m_listenfd, &m_rdset))
                {
                    int i = select_sub_process();

                    if (i == -1)
                    {
                        m_stop = true;
                        break;
                    }

                    struct sockaddr_in client_address;
                    socklen_t client_addrlength = sizeof(client_address);
                    int connfd = accept(m_listenfd, (struct sockaddr*)&client_address, &client_addrlength);
                    if (connfd < 0)
                    {
                        // printf("accept fail, errno is :%d\n", errno);
                        continue;
                    }

                    send_fd(m_sub_process[i].m_pipefd[0], connfd);
                    printf("send new connect fd to child:%d\n", i, iRet);
                    close(connfd);
                }

                if (FD_ISSET(sig_pipefd[0], &m_rdset))
                {
                    int sig;
                    int ret;
                    char signals[1024];
                    ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                    if (ret <= 0)
                    {
                        continue;
                    }
                    else
                    {
                        for (int i=0; i<ret; i++)
                        {
                            switch(signals[i])
                            {
                                case SIGCHLD:
                                {
                                    bool bHealthy = check_children_health();
                                    m_stop = !bHealthy; 
                                    break;
                                }
                                case SIGTERM:
                                case SIGINT:
                                {
                                    printf("Killing all of the children now...\n");
                                    kill_children();
                                    break;
                                }
                                default:
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void run_child()
    {
        struct timeval tv;
        m_user_chain = new chain<T>();
        setup_sig_pipe();
        prepare_multiplexing();
        int iRet = -1;

        int pipefd = m_sub_process[m_idx].m_pipefd[1];
        while (!m_stop)
        {
            tv.tv_sec = 10;
            tv.tv_usec = 0;
            update_fdset();

            iRet = select(m_maxfd+1, &m_rdset, NULL, &m_exceptset, &tv);
            if (iRet < 0)
            {
                if (errno != EINTR)
                {
                    printf("Child select exception, errno=%d!\n", errno);
                    break;
                }
                continue;
            }
            else if (iRet == 0)
            {
                continue;
            }
            else
            {
                if (FD_ISSET(sig_pipefd[0], &m_rdset))
                {
                    int sig;
                    char signals[1024];
                    iRet = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                    if (iRet <= 0)
                    {
                        continue;
                    }
                    else
                    {
                        for(int i=0; i<iRet; i++)
                        {
                            switch(signals[i])
                            {
                                case SIGCHLD:
                                {
                                    pid_t pid;
                                    int stat;
                                    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
                                    {
                                        continue;
                                    }
                                    break;
                                }
                                case SIGTERM:
                                case SIGINT:
                                {
                                    m_stop = true;
                                    break;
                                }
                                default:
                                {
                                    break;
                                }
                            }
                        }
                    }
                }

                int pipefd = m_sub_process[m_idx].m_pipefd[1];
                if (FD_ISSET(pipefd, &m_rdset))
                {
                    int connfd = recv_fd(pipefd);
                    T* t = new T();
                    m_maxfd = M_MAX(m_maxfd, connfd);
                    t->init(connfd);
                    chain_node<T> *pNode = new chain_node<T>(t);
                    m_user_chain->tail_insert(pNode);
                }

                chain_node<T> *node = m_user_chain->head();
                chain_node<T> *tmpNode = NULL;
                while (node != NULL)
                {
                    tmpNode = NULL;
                    if (FD_ISSET(node->data()->sockfd(), &m_rdset))
                    {
                        node->data()->process();
                    }

                    if (FD_ISSET(node->data()->sockfd(), &m_exceptset))
                    {
                        printf("client connection exception received!\n");
                        tmpNode = node;
                    }
                    node = node->next();
                    if (tmpNode != NULL)
                    {
                        m_user_chain->remove(tmpNode);
                    }
                }
            }
        }
    }

    void update_fdset();
    bool is_parent();


private:
    static ppool_select<T> * m_instance;
    fd_set m_rdset;
    fd_set m_exceptset;
    int m_maxfd;
    chain<T> *m_user_chain;
};

template< typename T>
ppool_select<T> * ppool_select<T>::m_instance = NULL;

template< typename T>
bool ppool_select<T>::is_parent()
{
    return (m_idx == -1);
}

template< typename T>
void ppool_select<T>::update_fdset()
{
    FD_ZERO(&m_rdset);
    FD_ZERO(&m_exceptset);

    FD_SET(sig_pipefd[0], &m_rdset);
    FD_SET(sig_pipefd[0], &m_exceptset);

    if (is_parent())
    {
        // In parent process
        FD_SET(m_listenfd, &m_rdset);
        FD_SET(m_listenfd, &m_exceptset);
    }
    else
    {
        // In child process, listen pipefd and client fd
        int pipefd = m_sub_process[m_idx].m_pipefd[1];
        FD_SET(pipefd, &m_rdset);
        FD_SET(pipefd, &m_exceptset);

        chain_node<T> *node = m_user_chain->head();
        chain_node<T> *tmpNode = NULL;
        while (node != NULL)
        {
            tmpNode = NULL;
            if (node->data()->isFree())
            {
                // printf("node is free, add into select list, fd:%d!\n", node->data()->sockfd());
                FD_SET(node->data()->sockfd(), &m_rdset);
                FD_SET(node->data()->sockfd(), &m_exceptset);
            }
            else if (node->data()->isClosed())
            {
                // printf("node closed, remove it from chain, fd:%d!\n", node->data()->sockfd());
                tmpNode = node;
            }
            node = node->next();
            if (tmpNode != NULL)
            {
                m_user_chain->remove(tmpNode);
            }
        }
    }
}


#endif
