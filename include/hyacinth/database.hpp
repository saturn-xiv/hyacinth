#pragma once

#include <chrono>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/trivial.hpp>

#include <libpq-fe.h>
#include <mysql/mysql.h>
#include <sqlite3.h>

namespace hyacinth {
struct Migration {
  uint64_t version;
  std::string name;
  std::string up;
  std::string down;
  std::optional<std::string> run_at;

  static const char SEP = '-';
  inline static const std::string UP = "up.sql";
  inline static const std::string DOWN = "down.sql";
};
class Driver {
 public:
  Driver(const std::string& migrations_table)
      : _migrations_table(migrations_table) {}
  virtual void migrate(const Migration& it) = 0;
  virtual void rollback(const Migration& it) = 0;
  virtual std::string create() = 0;
  virtual std::string drop() = 0;
  virtual std::vector<Migration> status() = 0;
  virtual void dump() = 0;
  virtual std::string restore() = 0;
  virtual std::pair<std::string, std::string> generate() = 0;
  virtual std::optional<Migration> get_migration(uint64_t version) = 0;
  virtual void insert_migration(uint64_t version, const std::string& name,
                                const std::string& up,
                                const std::string& down) = 0;
  virtual void update_migration(uint64_t version, const std::string& name,
                                const std::string& up,
                                const std::string& down) = 0;
  virtual void init_migrations_table() = 0;

 protected:
  std::string _migrations_table;
};

struct PostgreSqlConnection {
  PostgreSqlConnection(const std::string& uri) : db(PQconnectdb(uri.c_str())) {}
  ~PostgreSqlConnection() { PQfinish(this->db); }
  inline bool is_open() { return PQstatus(this->db) == CONNECTION_OK; }
  PGconn* db;
};
// https://www.postgresql.org/docs/current/libpq.html
class PostgreSql : public Driver {
 public:
  PostgreSql(const std::string& host, uint16_t port, const std::string& user,
             const std::optional<std::string> password,
             const std::string& db_name, const std::string& migrations_table)
      : Driver(migrations_table),
        _host(host),
        _port(port),
        _user(user),
        _password(password),
        _db_name(db_name) {}
  inline std::shared_ptr<PostgreSqlConnection> open() {
    const auto u = this->uri();
    BOOST_LOG_TRIVIAL(debug) << "open postgresql: " << u;
    std::shared_ptr<PostgreSqlConnection> it =
        std::make_shared<PostgreSqlConnection>(u);
    if (it->is_open()) {
      return it;
    }
    BOOST_LOG_TRIVIAL(error)
        << "connection to database failed: " << PQerrorMessage(it->db);
    return nullptr;
  }
  //   https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-CONNSTRING
  std::string uri();
  void migrate(const Migration& it) override;
  void rollback(const Migration& it) override;
  std::string create() override;
  std::string drop() override;
  std::vector<Migration> status() override;
  void dump() override;
  std::string restore() override;
  std::pair<std::string, std::string> generate() override;
  std::optional<Migration> get_migration(uint64_t version) override;
  void insert_migration(uint64_t version, const std::string& name,
                        const std::string& up,
                        const std::string& down) override;
  void update_migration(uint64_t version, const std::string& name,
                        const std::string& up,
                        const std::string& down) override;
  void init_migrations_table() override;

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
