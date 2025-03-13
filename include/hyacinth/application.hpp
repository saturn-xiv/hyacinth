#pragma once

#include "hyacinth/database.hpp"

#include <boost/filesystem.hpp>

namespace hyacinth {
class Application {
 public:
  Application() {}
  void launch(int argc, char* argv[]);

 private:
  void load(std::shared_ptr<Driver> engine,
            const boost::filesystem::path& migrations_dir);
};

}  // namespace hyacinth
