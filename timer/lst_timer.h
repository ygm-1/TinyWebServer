#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <time.h>
#include "../log/log.h"

class util_timer; // 前向声明
//用户数据结构  .客户端socket地址和文件描述符 
struct client_data {
    sockaddr_in address;
    int sockfd;
    util_timer * timer;
};

class util_timer {
public:
    util_timer() : prev(NULL), next(NULL) {}

public:
    time_t expire; //超时时间
    void (* cb_func)(client_data*); //任务回调函数
    client_data *user_data; 
    util_timer *prev;
    util_timer *next;
};
//定时器链表 升序双向链表 有头尾节点
class sort_timer_lst {
public:
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(util_timer *timer);
    //定时器任务发生变化 一般是延长 需要往后调整以保持升序
    void adjust_timer(util_timer *timer);
    void del_timer(util_timer *timer);
    void tick();

private:
    void add_timer(util_timer *timer, util_timer *lst_head);
    util_timer *head;
    util_timer *tail;
};

class Utils {
public:
    Utils(){};
    ~Utils() {};
    void init(int timeslot);
    //非阻塞
    int setnonblocking(int fd);
    //往内核事件表注册读写 ET epolloneshot
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);
    //信号处理函数
    staric void sig_handler(int sig);
    //设置信号函数
    void addsig(int sig, void(hanlder)(int), bool restart = true);
    //定时处理任务， 重新定时以不断触发SIGNALRM
    void timer_handler();
    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    sort_timer_lst m_timer_lst;
    static int u_epollfd;
    int m_TIMESLOT;
}

void cb_func(client_data *user_data);

#endif