
#include "ppool.h"
#include "utils.h"

processpool::processpool(int listenfd, int process_number)
    :m_listenfd(listenfd), m_process_number(process_number), m_idx(-1), m_stop(false), m_cur_sub_process_idx(0)
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

processpool::~processpool()
{
    delete [] m_sub_process;
}

void processpool::run()
{
    if (m_idx != -1)
    {
        run_child();
        return;
    }
    run_parent();
}

void processpool::setup_sig_pipe()
{
    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    assert(ret != -1);

    setnonblocking(sig_pipefd[1]);

    addsig(SIGCHLD, sig_handler);
    addsig(SIGTERM, sig_handler);
    addsig(SIGINT, sig_handler);
    addsig(SIGPIPE, SIG_IGN);
}

int processpool::select_sub_process()
{
    int i = m_cur_sub_process_idx;
    do
    {
        if (m_sub_process[i].m_pid != -1)
        {
            break;
        }
        i = (i+1)%m_process_number;
    } while(i != m_cur_sub_process_idx);

    if (m_sub_process[i].m_pid == -1)
    {
        return -1;
    }

    m_cur_sub_process_idx = (i+1)%m_process_number;
    return i;
}

bool processpool::check_children_health()
{
    pid_t pid;
    int stat;
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

    for (int j=0; j<m_process_number; j++)
    {
        if (m_sub_process[j].m_pid != -1)
        {
            return true;
        }
    }

    return false;
}

void processpool::kill_children()
{
    pid_t pid;
    for (int i=0; i<m_process_number; i++)
    {
        pid = m_sub_process[i].m_pid;
        if (pid != -1)
        {
            kill(pid, SIGTERM);
        }
    }
}


