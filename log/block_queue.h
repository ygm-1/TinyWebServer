//循环数组实现的阻塞队列， m_back = (m_back + 1) % m_max_size;
//线程安全，每个操作都现都先加互斥锁， 操作完成解锁


#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H


#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../lock/locker.h"
using namespace std

template <class T> 
class block_queue {
public:
    block_queue(int max_size = 1000) {
        if (max_size <= 0) {
            exit(-1);
        }

        m_max_size = max_size;
        m_array = new T[max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }

    void clear() {
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }

    ~block_queue() {
        m_mutex.lock();
        if (m_array != NULL) 
            delete[] m_array;
        m_mutex.unlock();
    }

    bool full() {
        m_mutex.lock();
        if (m_size >= m_max_size) {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    bool empty() {
        m_mutex.lock();
        if (m_size == 0) {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    bool front(T &value) {
        m_mutex.lock();
        if (m_size == 0) {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }

    bool back(T &value) {
        m_mutex.lock();
        if (m_size == 0) {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_back];
        m_mutex.unlock();
        return true;
    }

    int size() {
        int tmp = 0;
        m_mutex.lock();
        tmp = m_size;
        m_mutex.unlock();
        return tmp;
    }

    int max_size() {
        int tmp = 0;
        m_mutex.lock();
        tmp = m_max_size;
        m_mutex.unlock();
        return tmp;
    }
    //往队列添加元素时需要把所有使用队列的线程唤醒
    //是相当于生产者生产了一个元素
    //若无线程在等待条件变量 唤醒无意义
    bool push(const T &item) {
        m_mutex.lock();
        if (m_size >= m_max_size) {
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }
        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = item;
        m_size++;
        m_cond.broadcast();
        m_mutex.unlock();
        return true;
    }

    //pop时 如果队列为空 需要等待
    bool pop(T &item) {
        m_mutex.lock();
        //无超时处理则while
        while (m_size <= 0) {
            //cond_wait时会先把调用线程放入条件变量的等待队列里
            //然后将mutex解锁！！！， 线程放入队列之前，pthread_cond_broad_cast /signal 不会修改条件变量
            //即线程不会错过条件变量的任何变化， cond_wait成功返回时mutex再次上锁
            if (!m_cond.wait(m_nutex.get())) {
                m_mutex.unlock();
                return false;
            }
        }

        m_front = (m_front + 1) & m_max_size;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return false;
    }

    //超时处理
    bool pop(T & item, int ms_timeout) {
        struct timespec t = {0, 0};
        struct timeval = {0, 0};
        gettimeofday(&now, NULL);
        m_mutex.lock();
        if (m_size <= 0) {
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            if (!c_cond.timewait(m_mutex.get(), t)) {
                m_mutex.unlock();
                return false;
            }
        }
        if (m_size <= 0) {
            m_mutex.unlock();
            return false;
        }

        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }

private:
    locker m_mutex;
    cond m_cond;
    T *m_array;
    int m_size;
    int m_max_size;
    int m_front;
    int m_back;
};

#endif

