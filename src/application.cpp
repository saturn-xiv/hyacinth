#include "hyacinth/application.hpp"
#include "hyacinth/database.hpp"
#include "hyacinth/version.hpp"

#include <iostream>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <mysql/mariadb_version.h>

void hyacinth::Application::launch(int argc, char* argv[]) {
  boost::program_options::options_description generic("Generic options");
  generic.add_options()("help,h", "print help message")(
      "debug,d", "run on debug mode")("version,v", "print version info")(
      "config,c",
      boost::program_options::value<std::string>()->default_value("config.ini"),
      "configuration file")(
      "migrations-dir",
      boost::program_options::value<std::string>()->default_value("migrations"),
      "the directory containing migration files")(
      "migrations-table",
      boost::program_options::value<std::string>()->default_value(
          "schema_migrations"),
      "the database table to record migrations in")(
      "schema-file",
      boost::program_options::value<std::string>()->default_value("schema.sql"),
      "the schema file location");
  boost::program_options::options_description schema(
      "Database migration commands");
  schema.add_options()("new", "generate a new migration file");
  schema.add_options()("create", "create database");
  schema.add_options()("drop", "drop database");
  schema.add_options()("migrate", "migrate to the latest version");
  schema.add_options()("rollback", "rollback the most recent migration");
  schema.add_options()("status", "list applied and pending migrations");
  schema.add_options()("dump", "write the database schema and data to disk");

  boost::program_options::options_description all(
      hyacinth::PROJECT_DESCRIPTION);
  all.add(generic).add(schema);

  boost::program_options::variables_map vm;

  boost::program_options::store(
      boost::program_options::parse_command_line(argc, argv, all), vm);
  if (vm.count("help")) {
    std::cout << all << std::endl;
    return;
  }
  if (vm.count("version")) {
    std::cout << hyacinth::GIT_VERSION << std::endl;
    return;
  }
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      (vm.count("debug")
                                           ? boost::log::trivial::debug
                                           : boost::log::trivial::info));
  BOOST_LOG_TRIVIAL(debug) << "run on debug mode(" << hyacinth::GIT_VERSION
                           << ")";
  {
    const auto v = PQlibVersion();
    BOOST_LOG_TRIVIAL(debug) << "PostgreSQL v" << v / (100 * 100) << "."
                             << (v / 100) % 100 << "." << v % (100 * 100);
  }
  BOOST_LOG_TRIVIAL(debug) << "MySQL v" << MARIADB_CLIENT_VERSION_STR;
  BOOST_LOG_TRIVIAL(debug) << "Sqlite v" << SQLITE_VERSION;

  boost::property_tree::ptree tree;
  {
    const std::string config = vm["config"].as<std::string>();
    BOOST_LOG_TRIVIAL(info) << "load from " << config;
    boost::property_tree::ini_parser::read_ini(config, tree);
  }

  std::shared_ptr<hyacinth::Driver> engine;
  {
    const std::string driver = tree.get<std::string>("driver");
    const auto password = tree.get_optional<std::string>("postgresql.password");
    if (driver == "postgresql") {
      engine = std::make_shared<hyacinth::PostgreSql>(
          tree.get<std::string>("postgresql.host"),
          tree.get<uint16_t>("postgresql.port"),
          tree.get<std::string>("postgresql.user"),
          password ? std::optional<std::string>{*password} : std::nullopt,
          tree.get<std::string>("postgresql.db-name"));
    } else if (driver == "mysql") {
      // TODO
    } else if (driver == "sqlite3") {
      // TODO
    } else {
      BOOST_LOG_TRIVIAL(error) << "unsupported database " << driver;
      return;
    }
  }
  if (vm.count("new")) {
    // TODO
  }
  if (vm.count("dump")) {
    std::cout << engine->dump() << std::endl;
    return;
  }
  if (vm.count("create")) {
    std::cout << engine->create() << std::endl;
    return;
  }
  if (vm.count("drop")) {
    std::cout << engine->drop() << std::endl;
    return;
  }
  if (vm.count("status")) {
    // TODO
  }
  if (vm.count("migrate")) {
    // TODO
  }
  if (vm.count("rollback")) {
    // TODO
  }
  std::cout << schema << std::endl;
}
