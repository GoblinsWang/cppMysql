#ifndef MYSQLCLIENT_H
#define MYSQLCLIENT_H
#include "common.h"
#include "mysql_pool.h"

using std::pair;
using std::string;
using std::vector;
namespace cppmysql
{
    class MysqlClient
    {
    public:
        MysqlClient(const string &ip,
                    const string &user,
                    const string &passwd,
                    const string &dbName,
                    int port,
                    int minConn = 1,
                    int maxConn = 8);

        ~MysqlClient();
        // sql语句的执行函数
        vector<pair<string, string>> command(const string &cmd);

    private:
        string m_hostip;
        int m_hostport;
        int m_minConn;
        int m_maxConn;

        MysqlPool *m_mysqlPool;
    };

} // namespace cppmysql

#endif // MYSQLCLIENT_H