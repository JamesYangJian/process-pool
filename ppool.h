#ifndef _PPOOL_H__
#define _PPOOL_H__

#include <unistd.h>

class process
{
public:
    process() : m_pid(-1) {}

    pid_t m_pid;
    int m_pipefd[2];
};

class processpool
{
public:
    processpool(int listenfd, int process_number=8);
    ~processpool();

    void run();
    void setup_sig_pipe();
    int select_sub_process();
    bool check_children_health();
    void kill_children();
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
    int m_cur_sub_process_idx;
};

#endif