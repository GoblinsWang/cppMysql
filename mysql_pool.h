#ifndef MYSQL_POOL_H
#define MYSQL_POOL_H

#include <list>
#include <new>
#include <stdint.h>
#include <unistd.h>
#include <stdint.h>
#include <thread>
#include <functional>
#include <chrono>

#include "common.h"
#include "mysql_synch.h"

using std::string;

class MysqlPool;

class MysqlConnection
{
public:
    using ptr = std::shared_ptr<MysqlConnection>;

    MysqlConnection(MysqlPool *MysqlPool, int64_t timeout = 0);

    ~MysqlConnection();

    bool connect();

    void updateActiveTime();

    bool isExpire();

public:
    MYSQL *m_conn;

private:
    std::chrono::steady_clock::time_point m_lastActiveTime;

    MysqlPool *m_mysqlPool;

    int64_t m_timeout; // 默认该连接失效时间，为0时不失效
};

class MysqlPool
{
public:
    MysqlPool(const string &ip,
              const string &user,
              const string &passwd,
              const string &dbName,
              int port,
              int minConn,
              int maxConn);

    ~MysqlPool();

    int init();

    void serverCron();

    MysqlConnection::ptr getConnection();

    void freeConnection(MysqlConnection::ptr conn);

public:
    string m_hostip; // 主机ip
    string m_user;   // mysql用户名
    string m_passwd; // mysql密码
    string m_dbName; // 要使用的mysql数据库名字
    int m_hostport;  // mysql端口

private:
    int m_minConn; // 连接池允许最小连接数
    int m_maxConn; // 连接池允许最大连接数
    int m_curConn; // 连接池当前连接数

    mutable MutexLock m_mutex;
    Condition m_notEmpty;

    std::list<MysqlConnection::ptr> m_connQueue;
    std::thread *m_cronThread; //用于定时检测连接情况的线程
    bool m_quit;
};
#endif // Mysql_POOL_H
