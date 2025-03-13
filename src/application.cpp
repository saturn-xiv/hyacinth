#include "hyacinth/application.hpp"
#include "hyacinth/utils.hpp"
#include "hyacinth/version.hpp"

#include <iostream>
#include <regex>
#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/range/iterator_range.hpp>

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
  schema.add_options()("new", boost::program_options::value<std::string>(),
                       "generate a new migration file");
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

  const boost::filesystem::path migrations_dir(
      vm["migrations-dir"].as<std::string>());
  const std::string migrations_table = vm["migrations-table"].as<std::string>();
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
          tree.get<std::string>("postgresql.db-name"), migrations_table);
    } else {
      BOOST_LOG_TRIVIAL(error) << "unsupported database " << driver;
      return;
    }
  }

  if (vm.count("new")) {
    const auto name = vm["new"].as<std::string>();
    {
      static std::regex ex(R"(^[A-Za-z0-9]{2,128}$)");
      if (!std::regex_match(name, ex)) {
        BOOST_LOG_TRIVIAL(error) << "invalid migration name '" << name << "'";
        return;
      }
    }
    const auto version = hyacinth::timestamp();
    BOOST_LOG_TRIVIAL(info)
        << "generate migration " << version << hyacinth::Migration::SEP << name;

    const auto [up, down] = engine->generate();
    nlohmann::json data = {{"version", version}, {"name", name}};
    const boost::filesystem::path root =
        migrations_dir / (version + hyacinth::Migration::SEP + name);
    boost::filesystem::create_directories(root);
    hyacinth::render(root / hyacinth::Migration::UP, up, data);
    hyacinth::render(root / hyacinth::Migration::DOWN, down, data);
    return;
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
    this->load(engine, migrations_dir);
    auto items = engine->status();

    std::cout << std::setw(16) << std::left << "VERSION" << std::setw(32)
              << std::left << "NAME" << std::setw(28) << std::left << "RUN AT"
              << std::endl;

    for (const auto& it : items) {
      std::cout << std::setw(16) << std::left << it.version << std::setw(32)
                << std::left << it.name << std::setw(28) << std::left
                << it.run_at.value_or("n/a") << std::endl;
    }
    return;
  }
  if (vm.count("migrate")) {
    this->load(engine, migrations_dir);
    auto items = engine->status();
    for (const auto& it : items) {
      if (it.run_at) {
        continue;
      }
      engine->migrate(it);
    }
    return;
  }
  if (vm.count("rollback")) {
    this->load(engine, migrations_dir);
    auto items = engine->status();
    std::sort(items.begin(), items.end(), [](const auto& x, const auto& y) {
      return x.version > y.version;
    });
    for (const auto& it : items) {
      if (it.run_at) {
        engine->rollback(it);
        return;
      }
    }
    return;
  }
  std::cout << schema << std::endl;
}

void hyacinth::Application::load(
    std::shared_ptr<hyacinth::Driver> engine,
    const boost::filesystem::path& migrations_dir) {
  BOOST_LOG_TRIVIAL(debug) << "check migration table";
  engine->init_migrations_table();
  for (auto& entry : boost::make_iterator_range(
           boost::filesystem::directory_iterator(migrations_dir), {})) {
    const auto root = entry.path();
    const std::string file = root.filename().string();

    if (file.length() < 15 || file.at(14) != hyacinth::Migration::SEP) {
      throw std::invalid_argument("invalid migration folder " + file);
    }
    const uint64_t version = std::stol(file.substr(0, 14));
    const std::string name = file.substr(15);
    BOOST_LOG_TRIVIAL(debug)
        << "found migration " << version << "(" << name << ")";
    std::string up;
    {
      hyacinth::load(root / hyacinth::Migration::UP, up);
      boost::trim(up);
    }
    std::string down;
    {
      hyacinth::load(root / hyacinth::Migration::DOWN, down);
      boost::trim(down);
    }

    auto mig = engine->get_migration(version);
    if (mig) {
      if (mig->name == name && mig->up == up && mig->down == down) {
        continue;
      }
      if (mig->run_at) {
        throw std::invalid_argument("dirty migration " + file);
      }
      BOOST_LOG_TRIVIAL(warning) << "sync migration " << file;
      engine->update_migration(version, name, up, down);
    } else {
      BOOST_LOG_TRIVIAL(info) << "load migration " << file;
      engine->insert_migration(version, name, up, down);
    }
  }
}
