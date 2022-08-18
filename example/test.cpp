#include "mysql_client.h"
using namespace cppmysql;

int main()
{
  MysqlClient client("172.23.128.1", "root", "123456", "test", 0, 1, 8);
  auto m = client.command("select * from test where userId = 'lyz'");
  for (auto it = m.begin(); it != m.end(); ++it)
  {
    std::cout << it->first << std::endl;
    const std::string field = it->first;
    for (size_t i = 0; i < it->second.size(); i++)
    {
      std::cout << it->second[i] << std::endl;
    }
  }

  m = client.command("select * from test where userId = 'wzm'");
  for (auto it = m.begin(); it != m.end(); ++it)
  {
    std::cout << it->first << std::endl;
    const std::string field = it->first;
    for (size_t i = 0; i < it->second.size(); i++)
    {
      std::cout << it->second[i] << std::endl;
    }
  }
  return 0;
}
