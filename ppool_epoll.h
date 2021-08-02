#ifndef PPOOL_EPOLL_H__
#define PPOOL_EPOLL_H__

#include "ppool.h"

template< typename T>
class ppool_epoll : public processpool
{
private:
    ppool_epoll(int listenfd, int process_number=8):processpool(listenfd, process_number)
    {

    }

public:
    static ppool_epoll<T> * create(int listenfd, int process_number=8)
    {
        if (m_instance == NULL)
        {
            m_instance = new ppool_epoll<T>(listenfd, process_number);
        }
        return m_instance;
    }

private:
    void prepare_multiplexing()
    {
        m_epollfd = epoll_create(5);
        assert(m_epollfd != -1);
        addfd(m_epollfd, sig_pipefd[0]);
    }

    void run_parent()
    {
        setup_sig_pipe();
        prepare_multiplexing();

        addfd(m_epollfd, m_listenfd);

        epoll_event events[MAX_EVENT_NUMBER];
        int sub_process_counter = 0;
        int new_conn = 1;
        int number = 0;
        int ret = -1;

        while(!m_stop)
        {
            number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
            if ((number < 0) && (errno != EINTR))
            {
                printf("epoll failuer, errno:%d\n", errno);
                break;
            }

            for (int i=0; i<number; i++)
            {
                int sockfd = events[i].data.fd;
                if (sockfd == m_listenfd)
                {
                    int i = sub_process_counter;
                    do
                    {
                        if (m_sub_process[i].m_pid != -1)
                        {
                            break;
                        }
                        i = (i+1)%m_process_number;
                    } while(i != sub_process_counter);

                    if (m_sub_process[i].m_pid == -1)
                    {
                        m_stop = true;
                        break;
                    }

                    sub_process_counter = (i+1)%m_process_number;
                    ret = send(m_sub_process[i].m_pipefd[0], (char *)&new_conn, sizeof(new_conn), 0);
                    printf("send new connect request to child:%d, ret:%d\n", i, ret);
                }
                else if ((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN))
                {
                    int sig;
                    char signals[1024];
                    ret = recv(sockfd, signals, sizeof(signals), 0);
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
                                    pid_t pid;
                                    int stat;
                                    printf("SIGCHLD received in paraent...\n");
                                    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
                                    {
                                        for (int j=0; j<m_process_number; j++)
                                        {
                                            if (m_sub_process[j].m_pid == pid)
                                            {
                                                printf("Child process %d exit!\n", pid);
                                                close(m_sub_process[j].m_pipefd[0]);
                                                m_sub_process[j].m_pid = -1;
                                            }
                                        }
                                    }

                                    m_stop = true;
                                    for (int j=0; j<m_process_number; j++)
                                    {
                                        if (m_sub_process[j].m_pid != -1)
                                        {
                                            m_stop = false;
                                            break;
                                        }
                                    }
                                    break;
                                }
                                case SIGTERM:
                                case SIGINT:
                                {
                                    printf("Killing all of the children now...\n");
                                    for (int j=0; j<m_process_number; j++)
                                    {
                                        int pid = m_sub_process[i].m_pid;
                                        if (pid != -1)
                                        {
                                            kill(pid, SIGTERM);
                                        }
                                    }
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
                else
                {
                    continue;
                }
            }
        }
        close(m_epollfd);
    }

    void run_child()
    {
        setup_sig_pipe();
        prepare_multiplexing();

        int pipefd = m_sub_process[m_idx].m_pipefd[1];
        addfd(m_epollfd, pipefd);
        epoll_event events[MAX_EVENT_NUMBER];
        T* users = new T[USER_PER_PROCESS];
        assert(users);
        int number = 0;
        int ret = -1;

        while (!m_stop)
        {
            number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
            if ((number <0) && (errno != EINTR))
            {
                printf("epoll error!\n");
                break;
            }

            for (int i=0; i<number; i++)
            {
                int sockfd = events[i].data.fd;
                if ((sockfd == pipefd) && (events[i].events & EPOLLIN))
                {
                    int client = 0;
                    ret = recv(sockfd, (char *)&client, sizeof(client), 0);
                    if (((ret < 0) && (errno != EAGAIN)) || ret == 0)
                    {
                        continue;
                    }
                    else
                    {
                        struct sockaddr_in client_address;
                        socklen_t client_addrlength = sizeof(client_address);
                        int connfd = accept(m_listenfd, (struct sockaddr*)&client_address, &client_addrlength);
                        if (connfd < 0)
                        {
                            printf("accept fail, errno is :%d\n", errno);
                            continue;
                        }
                        addfd(m_epollfd, connfd);
                        users[connfd].init(connfd, client_address, m_epollfd);
                    }
                }
                else if((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN))
                {
                    int sig;
                    char signals[1024];
                    ret = recv(sockfd, signals, sizeof(signals), 0);
                    if (ret <= 0)
                    {
                        continue;
                    }
                    else
                    {
                        for(int i=0; i<ret; i++)
                        {
                            switch(signals[i])
                            {
                                case SIGCHLD:
                                {
                                    pid_t pid;
                                    int stat;
                                    printf("SIGCHLD received!\n");
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
                else if (events[i].events & EPOLLIN)
                {
                    users[sockfd].process();
                }
                else
                {
                    continue;
                }
            }
        }

        delete [] users;
        users = NULL;
        close(pipefd);
        close(m_epollfd);
    }


private:
    int m_epollfd;
    static ppool_epoll<T> * m_instance;
};

template< typename T>
ppool_epoll<T> * ppool_epoll<T>::m_instance = NULL;


#endif
