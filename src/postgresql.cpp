#include "hyacinth/database.hpp"

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
void hyacinth::PostgreSql::migrate() {
  // TODO
}
void hyacinth::PostgreSql::rollback() {
  // TODO
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
  // TODO
  return items;
}
std::string hyacinth::PostgreSql::dump() {
  const std::string tpl = R"RAW(
$ pg_dump -F custom --compress=9 -O -h {{ host }} -p {{ port }} -U {{ user }} -W {{ password }} -d {{ db_name }}
)RAW";
  nlohmann::json data = {{"host", this->_host},
                         {"port", this->_port},
                         {"user", this->_user},
                         {"password", this->_password.value_or("")},
                         {"db_name", this->_db_name}};
  return inja::render(tpl, data);
}
