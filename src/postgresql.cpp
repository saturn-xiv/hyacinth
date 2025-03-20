#include "hyacinth/database.hpp"
#include "hyacinth/utils.hpp"

#include <inja/inja.hpp>

std::string hyacinth::PostgreSql::uri() {
  std::stringstream ss;
  ss << "host=" << this->_host << " port=" << this->_port
     << " user=" << this->_user;
  if (this->_password) {
    ss << " password=" << this->_password.value();
  }
  ss << " dbname=" << this->_db_name << " sslmode=disable";
  return ss.str();
}
void hyacinth::PostgreSql::migrate(const Migration& it) {
  const auto con = this->open();

  auto res = PQexec(con->db, "BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    std::stringstream ss;
    ss << "BEGIN command failed: " << PQerrorMessage(con->db);
    PQclear(res);
    throw std::runtime_error(ss.str());
  }
  PQclear(res);

  {
    res = PQexec(con->db, it.up.c_str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      std::stringstream ss;
      ss << PQerrorMessage(con->db);
      PQclear(res);
      throw std::runtime_error(ss.str());
    }
    PQclear(res);
  }

  {
    const std::string tpl =
        R"SQL(UPDATE {{ table }} SET version=version+1, run_at=CURRENT_TIMESTAMP WHERE id=$1)SQL";

    nlohmann::json data = {{"table", this->_migrations_table}};
    const std::string sql = inja::render(tpl, data);

    BOOST_LOG_TRIVIAL(debug) << sql;

    const auto id = std::to_string(it.version);
    const char* const param_values[] = {id.c_str()};
    const int param_lengths[] = {sizeof(id.c_str())};
    const int param_formats[] = {0};
    res = PQexecParams(con->db, sql.c_str(), 1, NULL, param_values,
                       param_lengths, param_formats, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      std::stringstream ss;
      ss << PQerrorMessage(con->db);
      PQclear(res);
      throw std::runtime_error(ss.str());
    }
    PQclear(res);
  }

  res = PQexec(con->db, "END");
  PQclear(res);
}
void hyacinth::PostgreSql::rollback(const Migration& it) {
  const auto con = this->open();

  auto res = PQexec(con->db, "BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    std::stringstream ss;
    ss << "BEGIN command failed: " << PQerrorMessage(con->db);
    PQclear(res);
    throw std::runtime_error(ss.str());
  }
  PQclear(res);

  {
    res = PQexec(con->db, it.down.c_str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      std::stringstream ss;
      ss << PQerrorMessage(con->db);
      PQclear(res);
      throw std::runtime_error(ss.str());
    }
    PQclear(res);
  }

  {
    const std::string tpl =
        R"SQL(UPDATE {{ table }} SET version=version+1, run_at=NULL WHERE id=$1)SQL";

    nlohmann::json data = {{"table", this->_migrations_table}};
    const std::string sql = inja::render(tpl, data);

    BOOST_LOG_TRIVIAL(debug) << sql;

    const auto id = std::to_string(it.version);
    const char* const param_values[] = {id.c_str()};
    const int param_lengths[] = {sizeof(id.c_str())};
    const int param_formats[] = {0};
    res = PQexecParams(con->db, sql.c_str(), 1, NULL, param_values,
                       param_lengths, param_formats, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      std::stringstream ss;
      ss << PQerrorMessage(con->db);
      PQclear(res);
      throw std::runtime_error(ss.str());
    }
    PQclear(res);
  }

  res = PQexec(con->db, "END");
  PQclear(res);
}
std::string hyacinth::PostgreSql::create() {
  const std::string tpl = R"RAW(
$ psql -h {{ host }} -p {{ port }} -U postgres
> CREATE USER {{ user }} WITH PASSWORD '{{ password }}';
> CREATE DATABASE {{ db_name }} WITH ENCODING = 'UTF8' OWNER {{ user }};
)RAW";
  nlohmann::json data = {{"host", this->_host},
                         {"port", this->_port},
                         {"user", this->_user},
                         {"password", this->_password.value_or("")},
                         {"db_name", this->_db_name}};
  return inja::render(tpl, data);
}
std::string hyacinth::PostgreSql::drop() {
  const std::string tpl = R"RAW(
$ psql -h {{ host }} -p {{ port }} -U postgres
> DROP DATABASE {{ db_name }};
)RAW";
  nlohmann::json data = {{"host", this->_host},
                         {"port", this->_port},
                         {"db_name", this->_db_name}};
  return inja::render(tpl, data);
}
std::vector<hyacinth::Migration> hyacinth::PostgreSql::status() {
  std::vector<hyacinth::Migration> items;
  const auto con = this->open();

  const std::string tpl =
      R"SQL(SELECT id, name, up, down, run_at FROM {{ table }} ORDER BY id ASC;)SQL";
  nlohmann::json data = {{"table", this->_migrations_table}};
  const std::string sql = inja::render(tpl, data);

  BOOST_LOG_TRIVIAL(debug) << sql;

  auto res = PQexec(con->db, sql.c_str());
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    std::stringstream ss;
    ss << "select migration table failed: " << PQresultErrorMessage(res);
    PQclear(res);
    throw std::runtime_error(ss.str());
  }
  BOOST_LOG_TRIVIAL(debug) << "get " << PQntuples(res)
                           << " tuples, each tuple has " << PQnfields(res)
                           << " fields";

  for (int i = 0; i < PQntuples(res); i++) {
    hyacinth::Migration it;
    {
      auto v = PQgetvalue(res, i, 0);
      it.version = std::stol(v);
    }
    it.name = PQgetvalue(res, i, 1);
    it.up = PQgetvalue(res, i, 2);
    it.down = PQgetvalue(res, i, 3);
    {
      auto v = PQgetvalue(res, i, 4);
      if (strlen(v) > 0) {
        it.run_at = std::optional<std::string>{v};
      }
    }
    items.push_back(it);
  }

  PQclear(res);
  return items;
}
void hyacinth::PostgreSql::dump() {
  const auto file = this->_db_name + "-" + hyacinth::timestamp() + ".gz";
  BOOST_LOG_TRIVIAL(info) << "dump " << this->_host << ":" << this->_port << "/"
                          << this->_db_name << " into " << file;
  std::vector<std::string> args = {"pg_dump", "-f",     file,
                                   "-F",      "custom", "--compress",
                                   "9",       "-O",     "--no-owner"};

  {
    std::stringstream ss;
    {
      ss << "postgresql://" << this->_user;
      if (this->_password) {
        ss << ":" << this->_password.value();
      }
      ss << "@" << this->_host << ":" << this->_port << "/" << this->_db_name;
    }
    args.push_back("-d");
    args.push_back(ss.str());
  }
  hyacinth::execute(args);
}

std::string hyacinth::PostgreSql::restore() {
  const std::string tpl = R"RAW(
$ pg_restore -O -h {{ host }} -p {{ port }} -U {{ user }} -W {{ password }} -d {{ db_name }} {{ db_name }}-yyyyMMddHHMMSS.gz
)RAW";
  nlohmann::json data = {
      {"host", this->_host},       {"port", this->_port},
      {"user", this->_user},       {"password", this->_password.value_or("")},
      {"db_name", this->_db_name},
  };
  return inja::render(tpl, data);
}

std::pair<std::string, std::string> hyacinth::PostgreSql::generate() {
  return {R"SQL(
CREATE TABLE T{{ version }}{{ name }}(
    id SERIAL PRIMARY KEY,    
    deleted_at TIMESTAMP WITHOUT TIME ZONE,    
    version INT NOT NULL DEFAULT 0,
    updated_at TIMESTAMP WITHOUT TIME ZONE NOT NULL,    
    created_at TIMESTAMP WITHOUT TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);
)SQL",
          R"SQL(
DROP TABLE T{{ version }}{{ name }};
)SQL"};
}

void hyacinth::PostgreSql::insert_migration(uint64_t version,
                                            const std::string& name,
                                            const std::string& up,
                                            const std::string& down) {
  const auto id = std::to_string(version);
  const auto con = this->open();

  auto res = PQexec(con->db, "BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    std::stringstream ss;
    ss << "BEGIN command failed: " << PQerrorMessage(con->db);
    PQclear(res);
    throw std::runtime_error(ss.str());
  }
  PQclear(res);

  {
    const std::string tpl =
        R"SQL(INSERT INTO {{ table }}(id, name, up, down, updated_at) VALUES($1, $2, $3, $4, CURRENT_TIMESTAMP);)SQL";
    nlohmann::json data = {{"table", this->_migrations_table}};
    const std::string sql = inja::render(tpl, data);

    BOOST_LOG_TRIVIAL(debug) << sql;
    const char* const param_values[] = {id.c_str(), name.c_str(), up.c_str(),
                                        down.c_str()};
    const int param_lengths[] = {sizeof(id.c_str()), sizeof(name.c_str()),
                                 sizeof(up.c_str()), sizeof(down.c_str())};
    const int param_formats[] = {0, 0, 0, 0};
    res = PQexecParams(con->db, sql.c_str(), 4, NULL, param_values,
                       param_lengths, param_formats, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      std::stringstream ss;
      ss << "insert into migration table failed: " << PQerrorMessage(con->db);
      PQclear(res);
      throw std::runtime_error(ss.str());
    }
    PQclear(res);
  }

  res = PQexec(con->db, "END");
  PQclear(res);
}
void hyacinth::PostgreSql::update_migration(uint64_t version,
                                            const std::string& name,
                                            const std::string& up,
                                            const std::string& down) {
  const auto id = std::to_string(version);
  const auto con = this->open();

  auto res = PQexec(con->db, "BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    std::stringstream ss;
    ss << "BEGIN command failed: " << PQerrorMessage(con->db);
    PQclear(res);
    throw std::runtime_error(ss.str());
  }
  PQclear(res);

  {
    const std::string tpl =
        R"SQL(UPDATE {{ table }} SET name=$2, up=$3, down=$4, version=version+1, updated_at=CURRENT_TIMESTAMP WHERE id=$1;)SQL";
    nlohmann::json data = {{"table", this->_migrations_table}};
    const std::string sql = inja::render(tpl, data);

    BOOST_LOG_TRIVIAL(debug) << sql;
    const char* const param_values[] = {id.c_str(), name.c_str(), up.c_str(),
                                        down.c_str()};
    const int param_lengths[] = {sizeof(id.c_str()), sizeof(name.c_str()),
                                 sizeof(up.c_str()), sizeof(down.c_str())};
    const int param_formats[] = {0, 0, 0, 0};
    res = PQexecParams(con->db, sql.c_str(), 4, NULL, param_values,
                       param_lengths, param_formats, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      std::stringstream ss;
      ss << "insert into migration table failed: " << PQerrorMessage(con->db);
      PQclear(res);
      throw std::runtime_error(ss.str());
    }
    PQclear(res);
  }

  res = PQexec(con->db, "END");
  PQclear(res);
}
void hyacinth::PostgreSql::init_migrations_table() {
  const auto con = this->open();

  auto res = PQexec(con->db, "BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    std::stringstream ss;
    ss << "BEGIN command failed: " << PQerrorMessage(con->db);
    PQclear(res);
    throw std::runtime_error(ss.str());
  }
  PQclear(res);

  {
    const std::string tpl = R"SQL(
CREATE TABLE IF NOT EXISTS {{ table }}(
  id BIGINT PRIMARY KEY,
  name VARCHAR(255) NOT NULL,
  up TEXT NOT NULL,
  down TEXT NOT NULL,
  run_at TIMESTAMP WITHOUT TIME ZONE,    
  version INT NOT NULL DEFAULT 0,
  updated_at TIMESTAMP WITHOUT TIME ZONE NOT NULL,    
  created_at TIMESTAMP WITHOUT TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS idx_{{ table }}_name ON {{ table }}(name);
)SQL";

    nlohmann::json data = {{"table", this->_migrations_table}};
    const std::string sql = inja::render(tpl, data);

    BOOST_LOG_TRIVIAL(debug) << sql;
    res = PQexec(con->db, sql.c_str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      std::stringstream ss;
      ss << "create migration table failed: " << PQerrorMessage(con->db);
      PQclear(res);
      throw std::runtime_error(ss.str());
    }
    PQclear(res);
  }

  res = PQexec(con->db, "END");
  PQclear(res);
}

std::optional<hyacinth::Migration> hyacinth::PostgreSql::get_migration(
    uint64_t version) {
  const auto id = std::to_string(version);
  const auto con = this->open();

  const std::string tpl =
      R"SQL(SELECT id, name, up, down, run_at FROM {{ table }} WHERE id=$1 LIMIT 1;)SQL";
  nlohmann::json data = {{"table", this->_migrations_table}};
  const std::string sql = inja::render(tpl, data);

  BOOST_LOG_TRIVIAL(debug) << sql << "id=" << id;

  const char* const param_values[] = {id.c_str()};
  const int param_lengths[] = {sizeof(id.c_str())};
  const int param_formats[] = {0};
  auto res = PQexecParams(con->db, sql.c_str(), 1, NULL, param_values,
                          param_lengths, param_formats, 0);

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    std::stringstream ss;
    ss << "select migration table by id failed: " << PQresultErrorMessage(res);
    PQclear(res);
    throw std::runtime_error(ss.str());
  }
  BOOST_LOG_TRIVIAL(debug) << "get " << PQntuples(res)
                           << " tuples, each tuple has " << PQnfields(res)
                           << " fields";
  if (PQntuples(res) == 0) {
    PQclear(res);
    return std::nullopt;
  }

  hyacinth::Migration it;
  {
    auto v = PQgetvalue(res, 0, 0);
    it.version = std::stol(v);
  }
  it.name = PQgetvalue(res, 0, 1);
  it.up = PQgetvalue(res, 0, 2);
  it.down = PQgetvalue(res, 0, 3);
  {
    auto v = PQgetvalue(res, 0, 4);
    if (strlen(v) > 0) {
      it.run_at = std::optional<std::string>{v};
    }
  }

  PQclear(res);
  return {it};
}
