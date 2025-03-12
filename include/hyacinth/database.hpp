#pragma once

#include <string>

#include <boost/log/trivial.hpp>

#include <mysql/mysql.h>
#include <pgsql/libpq-fe.h>
#include <sqlite3.h>

namespace hyacinth {
// https://www.postgresql.org/docs/current/libpq.html
class PostgreSql {
 public:
  PostgreSql();
  //   https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-CONNSTRING
  std::string uri();

 private:
};

// https://dev.mysql.com/downloads/c-api/
class MySql {
 public:
 private:
};

// https://www.sqlite.org/cintro.html
class Sqlite3 {
 public:
 private:
};
}  // namespace hyacinth
