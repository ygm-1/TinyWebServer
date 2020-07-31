#ifndef HTTPCONNECTION_N
#define HTTPCONNECTION_N
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
#include <map>


#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "..log/log.h"
//线程池的模板参数类
class http_conn {
public :
    //文件名的最大长度
    static const int FILENAME_LEN = 200;
    //缓冲区大小
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    //HTTP请求方法
    enum METHOD {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    //解析客户请求时主状态机所处的状态
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    //服务器处理HTTP请求的可能结果
    enum HTTP_CODE {
        //请求不完整 继续读取数据
        NO_REQUEST,
        GET_REQUSET,
        BAD_REQUSET,
        NO_RESOURCE,
        FORBIDDEN_REQUSET,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    //行的读取状态 从状态机
    enum LINE_STATUS {
        LINE_OK,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    // 初始化新接受的连接
    void init(int sockfd, const sockaddr_in& addr, char*, int, int, int, tring user, string passwd, string sqlname);
    //关闭连接
    void close_conn(bool real_close = true);
    //处理请求
    void process();
    //非阻塞读写
    bool read_once();
    bool write();
    sockaddr_in* get_address() {
        return &m_address;
    }
    void initmysql_result(connection_pool* connPool);
    int timer_flag;
    int improv;

private:
    //初始化连接
    void init();
    //解析请求
    HTTP_CODE process_read();
    //填充应答
    bool process_write(HTTP_CODE ret);
    // 由process_read调用以分析HTTP请求
    HTTP_CODE parse_request_line(char* text);
    HTTP_CODE parse_headers(char* text);
    HTTP_COdE parse_content(char* text);
    HTTP_CODE do_request();
    char* get_line() {return m_read_buf + m_start_line;};
    LINE_STATUS parse_line();
    //由process_write调用以填充HTTP应答
    void unmap();
    bool add_response(const char* format, ...);
    bool add_content(const char* content);
    bool add_status_line(int status, const char* title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    //所有socket上的事件被注册到同一个epoll事件表中 静态
    static int m_epollfd;
    //统计用户数量
    static int m_user_count;
    MYSQL* mysql;
    int m_state; // 读写状态

private:
    //该http连接的socket和对方的socket地址
    int m_sockfd;
    sockaddr_in m_address;
    //读缓冲去
    char m_read_buf[READ_BUFFER_SIZE];
    //标识已经读入的客户数据的下一个位置(字节)
    int m_read_idx;
    //正在分析的客户数据字符在读缓存中的位置
    int m_checked_idx;
    //当前解析的行的起始位置
    int m_start_line;
    char m_write_buf[WRITE_BUFFER_SIZE];
    //写缓存区中待发送的字节数
    int m_write_idx;
    //主状态机所处状态
    CHECK_STATE m_check_state;
    METHOD m_method; // 请求类型
    //客户请求的目标文件的完整路径 doc_root(网站根目录) + m_urls
    char m_real_file[FILENAME_LEN];
    //客户请求的目标文件的文件名
    char* m_url;
    //HTTP协议版本号 仅支持HTTP/1.1
    char* m_version;
    //主机名
    char* m_host;
    //请求的消息体长度
    int m_content_length;
    //http请求是否要求保持连接
    bool m_linger;
    //客户请求目标文件被mmap到内存中的起始位置
    char* m_file_address;
    //目标文件的状态
    struct stat m_file_stat;
    //writev来执行写操作
    struct iovec m_iv[2];
    int m_iv_count;
    int cgi; // 是否启用的POST
    char* m_string; //存储请求头数据
    int bytes_to_send;
    int bytes_have_send;
    //网站根目录
    char* doc_root;

    map<string, string> m_users;
    int m_TRIGMode;
    int m_close_log;

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];
};

#endif