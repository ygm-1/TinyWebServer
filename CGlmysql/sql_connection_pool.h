#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "..lock/locker.h"
#include "../log/log.h"

using namespace std;

class connection_pool {
public:
    MYSQL* GetConnection();  //获取数据库连接
    bool ReleaseConnection(MYSQL* conn); //release connection
    int GetFreeConn(); // get connection
    void DestroyPool(); // destroy all connection

    //单例模式
    static connection_pool* GetInstance();
    void init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn, int close_log);

private:
    connection_pool();
    ~connection_pool();
    
    int m_MaxConn;
    int m_CurConn;
    int m_FreeConn;
    locker lock;
    list<MYSQL*> connList; // connection poll
    sem reserve;

public: 
    string m_curl; // 主机地址
    string m_Port; //数据库端口号；
    string m_User;
    string m_PassWord;
    string m_DatabaseName; // 数用数据库名
    int m_close_log; //日志开关
};

class connectionRAII {
public:
    connectionRAII(MYSQL** con, connection_pool* connpool);
    ~connectionRAII();

private:
    MYSQL* conRAII;
    connection_pool* poolRAII;
};


#endif