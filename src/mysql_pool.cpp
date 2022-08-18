#include "mysql_pool.h"

MysqlConnection::MysqlConnection(MysqlPool *mysqlPool, int64_t timeout)
    : m_conn(nullptr),
      m_lastActiveTime(std::chrono::steady_clock::now()),
      m_mysqlPool(mysqlPool),
      m_timeout(timeout)
{
}

MysqlConnection::~MysqlConnection()
{
    if (m_conn)
    {
        mysql_close(m_conn);
        m_conn = nullptr;
    }
}

bool MysqlConnection::connect()
{
    m_conn = mysql_init(m_conn);
    if (m_conn == nullptr)
    {
        std::cout << "init failed" << std::endl;
        return false;
    }

    if (mysql_real_connect(m_conn,
                           m_mysqlPool->m_hostip.c_str(),
                           m_mysqlPool->m_user.c_str(),
                           m_mysqlPool->m_passwd.c_str(),
                           m_mysqlPool->m_dbName.c_str(),
                           m_mysqlPool->m_hostport,
                           nullptr,
                           0))
    {
        return true;
    }

    std::cout << mysql_error(m_conn) << std::endl;
    return false;
}

void MysqlConnection::updateActiveTime()
{
    if (m_timeout > 0)
        m_lastActiveTime = std::move(std::chrono::steady_clock::now());
}

bool MysqlConnection::isExpire()
{
    if (m_timeout > 0)
    {
        auto curTime = std::move(std::chrono::steady_clock::now());
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(curTime - m_lastActiveTime);
        if (duration.count() >= m_timeout)
            return true;
    }
    return false;
}

MysqlPool::MysqlPool(const string &ip,
                     const string &user,
                     const string &passwd,
                     const string &dbName,
                     int port,
                     int minConn,
                     int maxConn)
    : m_hostip(ip),
      m_user(user),
      m_passwd(passwd),
      m_dbName(dbName),
      m_hostport(port),
      m_minConn(minConn),
      m_maxConn(maxConn),
      m_curConn(0),
      m_mutex(),
      m_notEmpty(m_mutex),
      m_connQueue(),
      m_quit(false)
{
}

MysqlPool::~MysqlPool()
{
    MutexLockGuard lock(m_mutex);

    m_quit = true;
    m_cronThread->join();

    m_connQueue.clear();
    m_minConn = 0;
}

int MysqlPool::init()
{
    for (int i = 0; i < m_minConn; i++)
    {
        auto conn = std::make_shared<MysqlConnection>(this);
        if (conn->connect())
        {
            m_curConn++;
            m_connQueue.push_back(conn);
        }
    }

    m_cronThread = new std::thread(std::bind(&MysqlPool::serverCron, this));

    return 0;
}

// move out the disabled connections
void MysqlPool::serverCron()
{
    while (!m_quit)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10000)); // 10s检测一次
        MutexLockGuard lock(m_mutex);

        for (auto it = m_connQueue.begin(); it != m_connQueue.end(); it++)
        {
            if ((*it)->isExpire() && m_curConn > m_minConn)
            {
                m_curConn--;
                m_connQueue.remove(*it);
            }
        }
    }
}

MysqlConnection::ptr MysqlPool::getConnection()
{
    MutexLockGuard lock(m_mutex);

    while (m_connQueue.empty())
    {
        if (m_curConn >= m_maxConn)
        {
            m_notEmpty.wait();
        }
        else
        {
            auto conn = std::make_shared<MysqlConnection>(this, 120); // 可以为额外新增的连接设置有效期
            if (conn->connect())
            {
                m_connQueue.push_back(conn);
                m_curConn++;
            }
        }
    }

    auto pConn = m_connQueue.front();
    m_connQueue.pop_front();

    return pConn;
}

void MysqlPool::freeConnection(MysqlConnection::ptr conn)
{
    MutexLockGuard lock(m_mutex);
    auto it = m_connQueue.begin();
    while (it != m_connQueue.end())
    {
        if (*it == conn)
            break;
        it++;
    }

    if (it == m_connQueue.end())
    {
        m_connQueue.push_back(conn);
    }

    m_notEmpty.notify(); //这个场景下，notifyall不能用，释放一次，通知一次才对
}
