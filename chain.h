#ifndef _CHAIN_H__
#define _CHAIN_H__

#include<stdio.h>
#include <stdlib.h>
#include <string.h>

template <typename D>
class chain_node
{
public:
    chain_node(D * data)
    {
        m_prev = NULL;
        m_next = NULL;
        m_data = data;
    }

    ~chain_node()
    {
        delete m_data;
        m_data = NULL;
    }

    chain_node* next() { return m_next; }
    chain_node* prev() { return m_prev; }
    D* data() { return m_data; }

public:
    chain_node *m_prev;
    chain_node *m_next;
    D *m_data;
};

template <typename S>
class chain
{
public:
    chain();
    ~chain();

    chain_node<S> *head();
    chain_node<S> *tail();
    int count();
    void tail_insert(chain_node<S> *node);
    void remove(chain_node<S> *node);
    void release();

private:
    chain_node<S> *m_head;
    chain_node<S> *m_tail;
    int m_count;
};

template <typename S>
chain<S>::chain()
{
    m_head = NULL;
    m_tail = NULL;
    m_count = 0;
}

template <typename S>
chain_node<S> *chain<S>::head()
{
    return m_head;
}

template <typename S>
chain_node<S> *chain<S>::tail()
{
    return m_tail;
}
template <typename S>
int chain<S>::count()
{
    return m_count;
}

template <typename S>
void chain<S>::tail_insert(chain_node<S> *node)
{
    if (m_head == NULL)
    {
        m_head = node;
        m_tail = node;
    }
    else
    {
        m_tail->m_next = node;
        m_tail = node;
    }
    m_count ++;
}

template <typename S>
void chain<S>::remove(chain_node<S> *node)
{
    if (node->m_prev == NULL && node->m_next == NULL && node != m_head)
    {
        return;
    }

    if (m_head == node)
    {
        m_head = node->m_next;
    }
    else if (m_tail == node)
    {
        m_tail = node->m_prev;
        m_tail->m_next = NULL;
    }
    else
    {
        node->m_prev->m_next = node->m_next;
        node->m_next->m_prev = node->m_prev;
    }

    delete node;
    m_count --;
}

template <typename S>
void chain<S>::release()
{
    chain_node<S> *tmp_node = m_head;

    while (m_head != NULL)
    {
        m_head = m_head->m_next;
        delete tmp_node;
        tmp_node = m_head;
    }
    m_tail = NULL;
    m_count = 0;

}

#endif