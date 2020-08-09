#include "webserver.h"

WebServer::WebServer() {
    //http_conn类对象
    users = new http_conn[MAX_FD];

    //root文件夹路径
    char server_path[200];
    getcwd(server_path, 200);
    char root[6] = "/root";
    m_root = (char*)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root, server_path);
    strcat(m_root, root);

    user_timer = new client_data[MAX_FD];
}


WebServer::~WebServer() {
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    delete[] users;
    delete[] user_timer;
    delete m_pool;
}

void WebServer::init(int port, string user, string passWord, string databaseName, int log_write, 

                     int opt_linger, int trigmode, int sql_num, int thread_num, int close_log, int actor_model)

{
    m_port = port;
    m_user = user;
    m_passWord = passWord;
    m_databaseName = databaseName;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
    m_log_write = log_write;
    m_OPT_LINGER = opt_linger;
    m_TRIGMode = trigmode;
    m_close_log = close_log;
    m_actormodel = actor_model;
}

void WebServer::trig_mode() {
    //LT LT
    if (m_TRIGMode == 0) {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 0;
    }
    //LT ET
    else if (m_TRIGMode == 1) {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 1;
    }
    else if (2 == m_TRIGMode) {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 0;
    }
    //ET + ET
    else if (3 == m_TRIGMode) {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 1;
    }
}

void WebServer::log_write() {
    if (0 == m_close_log) {
        //初始化日志
        if (1 == m_log_write)
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 800);
        else
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 0);

    }
}

void WebServer::sql_pool() {
    //初始化数据库连接池
    m_connPool = connection_pool::GetInstance();
    m_connPool->init("localhost", m_user, m_passWord, m_databaseName, 3306, m_sql_num, m_close_log);
    //初始化数据库读取表
    users->initmysql_result(m_connPool);
}

void WebServer::thread_pool() {
    //线程池
    m_pool = new threadpool<http_conn>(m_actormodel, m_connPool, m_thread_num);
}

void WebServer::eventListen() {
    //确定socket协议族 服务类型
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);
    //LINGER控制close TCP连接的行为，默认关闭一个TCP连接时 close立即返回，并把发送缓存区的数据发送给对方。若为阻塞socket等待linger时间确认数据被接受 非阻塞 发送并返回
    if (m_OPT_LINGER == 0) {
        struct linger tmp = {0, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    }
    else if (m_OPT_LINGER == 1) {
        struct linger tmp = {1, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof address);
    //地址协议族名
    address.sinfamily = AF_INET；
    //将长整型主机字节序数据转化为网络字节序长整型
    //host to network long(长型转换IP地址，短型用来转换端口号)
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    int flag = 1;
    setsocketopt(m_listenfd, SQL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof address);
    assert(ret >= 0);
    ret = listen(m_listenfd, 5);
    assert(ret >= 0);
    
    utils.init(TIMESLOT);

    //用于存放返回的就绪事件表
    epoll_event events[MAX_EVENT_NUMBER];
    //EPOLL内核事件表
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);
    
    
    utils.addfd(m_epollfd, m_listenfd, false, m_LISTENTrigmode);
    http_conn::m_epollfd = m_epollfd;
    //创建全双工管道， PF+_UNIXPF+_UNIX本地协议族
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);
    utils.setnonblocking(m_pipefd[1]);
    //false /true 是否为oneshot
    utils.addfd(m_epollfd, m_pipefd[0], false, 0);
    //往读端关闭的管道或socket写
    //信号处理函数 统一信号源
    utils.addsig(SIGPIPE, SIG_IGN);
    //由alarm或定时器超时
    utils.addsig(SIGALRM, utils.sig_handler, false);
    //终止进程
    utils.addsig(SIGTERM, utils.sig_handler, false);
    alarm(TIMESLOT);
    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;
}


void WebServer::timer(int connfd, struct sockaddr_in client_address)
{
     users[connfd].init(connfd, client_address, m_root, m_CONNTrigmode, m_close_log, m_user, m_passWord, m_databaseName);
    //初始化client_data数据
    //创建定时器，设置回调函数和超时时间，绑定用户数据，
    //将定时器添加到链表中
    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;
    util_timer *timer = new util_timer;
    timer->user_data = &users_timer[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    user_timer[connfd].timer = timer;
    utils.m_timer_lst.add_timer(timer);
}

//若有数据传输，则将定时器往后延迟3个单位
//并对新的定时器在链表上的位置进行调整

void WebServer::adjust_timer(util_timer *timer) {
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    utils.m_timer_lst.adjust_timer(timer);
    LOG_INFO("%s", "adjust timer once"); 
}

void WebServer::deal_timer(util_timer *timer, int sockfd) {
    timer->cb_func(&users_timer[sockfd]);
    if (timer) {
        utils.m_timer_lst.del_timer(timer);
    }
    LOG_INFO("close fd %d", users_timer[sockfd].sockfd);
}


bool WebServer::dealclinetdata() {
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    if (0 == m_LISTENTrigmode) {
        int connfd - accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
        if (connfd < 0) {
            LOG_ERROR("%s:errno is:%d", "accept error", errno)
            return false;
        }
        if (http_conn::m_user_count >= MAX_FD) {
            utils.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        timer(connfd, client_address);
    }
    else {
        while (1) {
            int connfd = accept(m_listenfd, (struct sockaddr*)&client_address, &client_addrlenght);
            if (connfd < 0) {
                LOG_ERROR("%s:errno is:%d", "accept error", errno);
                break;
            }
            if (http_conn::m_user_count >= MAX_FD) {
                utils.show_error(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                break;
            }
            timer(connfd, client_address);
        }
        return false;
    }
    return true;
}


bool WebServer::dealwithsignal(bool &timeout, bool &stop_server) {
    int ret = 0;
    int sig;
    char signals[1024];
    ret = recv(m_pipefd[0], signals, sizeof signals, 0);
    if (ret == 1) return false;
    else if (ret == 0) return false;
    else {
        for (int i = 0; i < ret; i++) {
            switch(signals[i]) {
                case SIGALRM: {
                    timeout = true;
                    break;
                }
                case SIGTERM: {
                    stop_server = true;
                    break;
                }
            }
        }
    }
    return true;
}


void WebServer::dealwithread(int sockfd) {
    util_timer *timer = users_timer[sockfd].timer;
    //reactor
    if (m_actormodel == 1) {
        if (timer) adjust_timer(timer);
    }

    //若监测到读事件 将事件放入请求队列
    m_pool->append(users + sockfd, 0);
    while (true) {
        if (users[sockfd].improv == 1) {
            if (users[sockfd].timer_flag == 1) {
                deal_timer(timer, sockfd);
                users[sockfd].timer_flag = 0;
            }
            users[sockfd].improv = 0;
            break;
        }
    }
    else {
        //proactor
        if (users[sockfd].read_once()) {
            LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
            m_pool->append_p(user + sockfd);
            if (timer) adjust_timer(timer);
        }
        else deal_timer(timer, sockfd);
    }
}

void WebServer::dealwithwrite(int sockfd) {
    util_timer *timer = users_timer[sockfd].timer;
    //reactor
    if (m_actormodel == 1) {
        if (timer) adjust_timer(timer);
        m_pool->append(users + sockfd, 1);
        while (1) {
            if (users[sockfd].improv == 1) {
                if (users[sockfd].timer_flag == 1) {
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    else {
        //proactor
        if (users[sockfd].write()) {
            LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
            if (timer) adjust_timer(timer);        
        }
        else deal_timer(timer, sockfd);
    }
}

void WebServer::eventLoop() {
    bool timeout = false;
    bool stop_server = false;
    while (!stop_server) {
        //将内核事件表中就绪事件复制到events数组中返回就绪事件个数
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            LOG_ERROR("%s", "EPOLL failure")
            break;
        }
        //以此处理就绪事件
        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            //如果为新监听到的事件
            if (sockfd == m_listenfd) {
                bool flag = dealclinetdata();
                if (flag == false) continue;
            }
            else if (events[i].events &(EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 对方关闭TCP连接或者关闭写， 挂起 错误
                //服务器段也关闭， 移除定时器
                util_timer *timer = users_timer[sockfd].timer;
                deal_timer(timer, sockfd);
            }

            // 处理信号
            else if (sockfd == m_pipefd[0] && (events[i].events & EPOLLIN)) {
                bool flag = dealwithsignal(timeout, stop_server);
                if (flag == false)  LOG_ERROR("%s", "dealclientdata failure");
            }

            //处理客户连接收到的数据
            else if (events[i].events & EPOLLIN) {
                dealwithread(sockfd);
            }
            else if (events[i].events & EPOLLOUT) {
                dealwithwrite(sockfd);
            }
        }
        if (timeout) {
            utils.timer_handler();
            LOG_INFO("%s", "timer tick");
            timeout = false;
        }

    }
}