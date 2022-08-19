#include "../mysql_client.h"
using namespace cppmysql;

MysqlClient::MysqlClient(const string &ip,
                         const string &user,
                         const string &passwd,
                         const string &dbName,
                         int port,
                         int minConn,
                         int maxConn)
{
    m_hostip = ip;
    m_hostport = port;
    m_mysqlPool = new MysqlPool(ip, user, passwd, dbName, port, minConn, maxConn);
    m_mysqlPool->init();
    std::cout << "mysql pool init success!" << std::endl;
    // LogDebug("mysql pool init success!");
}

MysqlClient::~MysqlClient()
{
    if (m_mysqlPool)
    {
        delete m_mysqlPool;
        m_mysqlPool = nullptr;
    }
}

/*
 * sql语句执行函数，并返回结果，没有结果的SQL语句返回空结果，
 * 每次执行SQL语句都会先去连接队列中去一个连接对象，
 * 执行完SQL语句，就把连接对象放回连接池队列中。
 * 返回对象用map主要考虑，用户可以通过数据库字段，直接获得查询的字。
 * 例如：m["字段"][index]。
 */
vector<pair<string, string>>
MysqlClient::command(const string &sql)
{
    auto conn = m_mysqlPool->getConnection();

    vector<pair<string, string>> results;
    if (conn->m_conn)
    {
        if (mysql_query(conn->m_conn, sql.c_str()) == 0)
        {
            MYSQL_RES *res = mysql_store_result(conn->m_conn);
            if (res)
            {
                MYSQL_FIELD *field;
                while ((field = mysql_fetch_field(res)))
                {
                    // std::cout << "field:" << field->name << std::endl;
                    results.push_back(make_pair(field->name, ""));
                }
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)))
                {
                    unsigned int i = 0;
                    for (auto it = results.begin(); it != results.end(); it++)
                    {
                        it->second = row[i++];
                        //(it->second).push_back(row[i++]);
                    }
                }
                mysql_free_result(res);
            }
            else
            {
                if (mysql_field_count(conn->m_conn) != 0)
                    std::cerr << mysql_error(conn->m_conn) << std::endl;
            }
        }
        else
        {
            std::cerr << mysql_error(conn->m_conn) << std::endl;
        }
    }
    else
    {
        std::cerr << mysql_error(conn->m_conn) << std::endl;
    }
    m_mysqlPool->freeConnection(conn);
    return results;
}