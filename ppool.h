#ifndef _PPOOL_H__
#define _PPOOL_H__
#include "utils.h"

class processpool
{
public:
    processpool(int listenfd, int process_number=8)
        :m_listenfd(listenfd), m_process_number(process_number), m_idx(-1), m_stop(false)
    {
        assert ( (process_number > 0) && (process_number <= MAX_PROCESS_NUMBER) );
        m_sub_process = new process[process_number];
        assert(m_sub_process);

        for (int i=0; i<process_number; ++i)
        {
            int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd);
            assert(ret == 0);

            m_sub_process[i].m_pid = fork();
            assert(m_sub_process[i].m_pid >= 0);
            if (m_sub_process[i].m_pid > 0)
            {
                close(m_sub_process[i].m_pipefd[1]);
                continue;
            }
            else
            {
                close(m_sub_process[i].m_pipefd[0]);
                m_idx = i;
                break;
            }
        }
    }

public:
    ~processpool()
    {
        printf("Releasing sub process!\n");
        delete [] m_sub_process;
    }

    void run()
    {
        if (m_idx != -1)
        {
            run_child();
            return;
        }
        run_parent();
    }

public:
    void setup_sig_pipe()
    {
        int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
        assert(ret != -1);

        setnonblocking(sig_pipefd[1]);

        addsig(SIGCHLD, sig_handler);
        addsig(SIGTERM, sig_handler);
        addsig(SIGINT, sig_handler);
        addsig(SIGPIPE, SIG_IGN);
    }

    virtual void run_parent() = 0;
    virtual void run_child() = 0;


public:
    static const int MAX_PROCESS_NUMBER = 16;
    static const int USER_PER_PROCESS = 65536;
    static const int MAX_EVENT_NUMBER = 10000;
    int m_process_number;
    int m_idx;
    int m_listenfd;
    int m_stop;
    process *m_sub_process;
};

#endif