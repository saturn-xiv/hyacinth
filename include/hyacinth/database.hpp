#pragma once

#include <chrono>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/trivial.hpp>

#include <libpq-fe.h>
#include <mysql/mysql.h>
#include <sqlite3.h>

namespace hyacinth {
struct Migration {
  uint32_t version;
  std::string name;
  std::string up;
  std::string down;
  boost::posix_time::ptime* run_at;
};
class Driver {
 public:
  virtual void migrate() = 0;
  virtual void rollback() = 0;
  virtual std::string create() = 0;
  virtual std::string drop() = 0;
  virtual std::vector<Migration> status() = 0;
  virtual std::string dump() = 0;
};
// https://www.postgresql.org/docs/current/libpq.html
class PostgreSql : public Driver {
 public:
  PostgreSql(const std::string& host, uint16_t port, const std::string& user,
             const std::optional<std::string> password,
             const std::string& db_name)
      : _host(host),
        _port(port),
        _user(user),
        _password(password),
        _db_name(db_name) {}
  //   https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-CONNSTRING
  std::string uri();
  void migrate() override;
  void rollback() override;
  std::string create() override;
  std::string drop() override;
  std::vector<Migration> status() override;
  std::string dump() override;

 private:
  std::string _host;
  uint16_t _port;
  std::string _user;
  std::optional<std::string> _password;
  std::string _db_name;
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
