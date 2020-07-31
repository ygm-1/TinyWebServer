#include "lst_timer.h"
#include "../http/http_conn.h"

sort_timer_lst::sort_timer_lst() {
    head = NULL;
    tail = NUll;
}

sort_timer_lst::~sort_timer_lst() {
    util_timer *tmp = head;
    while (tmp) {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}


void sort_timer_lst::add_timer(util_timer *timer) {
    if (!timer) return;

    if (!head) {
        head = tail = timer;
        return;
    }
    if (timer->expire < head->expire) {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }

    add_timer(timer, head);
}

void sort_timer_lst::adjust_timer(util_timer *timer) {
    if (!timer) return ;
    util_timer * tmp = timer->next;
    //在尾部或者依然小于next不用调整
    if (!tmp || (timer->expire < tmp->expire)) return;
    //如果是头节点 取出后重新插入
    if (timer == head) {
        head = head->next;
        head->prev = NULL;
        timer->next = NULL;
        add_timer(timer, head);
    }
    //否则先取出后往 后面的链表插
    else {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(time, timer->next);
    }
}

void sort_timer_lst::del_timer(util_timer *timer) {
    if (!timer) return;
    if ((timer) == head && timer == tail) {
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }
    if (timer == head) {
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }
    if (timer == tail) {
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return NULL;
    }
    timer->perv->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
    return;
}

void sort_timer_lst::tick() {
    if (!head) return;
    time_t cur = time(NULL);
    util_timer *tem = head;
    while (tmp) {
        if (cur < tem->expire) break;
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if (head) {
            head->prev = NULL;
        }
        delete tmp;
        tmp = head;
    }
}


void sort_timer_lst::add_timer(util_timer *timer, util_timer *lst_head) {
    util_timer *cur = lst_head; //
    util_timer *tmp = cur->next;
    while (tmp) {
        if (timer->expire < tmp->expire) {
            cur->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = cur;
        }
        cur = tmp;
        tmp = tmp->next;
    }
    if (!tmp) {
        cur->next = timer;
        timer->prev = cur;
        timer->next = NULL;
        tail = timer;
    }
}


void Utils::init(int timeslot) {
    m_TIMESLOT - timeslot;
}

int Utils::setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//往内核事件表注册读事件
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode) {
    //event参数指定事件 是个结构体 包括epoll事件和用户数据
    epoll_event event;
    event.data.fd = fd;

    if (TRIGMode == 1) {
        //正在发生的事件的事件类型
        event.events = EPOLLIN | EPOLLET |EPOLLRDHUP;
    }
    else event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot) event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void Utils::addsig(int sig, void(handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void Utils::timer_handler(){
    m_timer_lst.tick();
    alarm(m_TIMESLOT);
}


void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}