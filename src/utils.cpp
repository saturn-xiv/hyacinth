#include "hyacinth/utils.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/log/trivial.hpp>

#include <inja/inja.hpp>

// https://cplusplus.com/reference/iomanip/put_time/
std::string hyacinth::timestamp(std::time_t it) {
  std::stringstream ss;
  struct std::tm* tm = std::localtime(&it);
  ss << std::put_time(tm, "%Y%m%d%H%M%S");
  return ss.str();
}

void hyacinth::render(const boost::filesystem::path& file,
                      const std::string& tpl, const nlohmann::json& data) {
  boost::filesystem::ofstream it;
  BOOST_LOG_TRIVIAL(debug) << "write file " << file.string();
  it.open(file);
  inja::render_to(it, tpl, data);
  it.close();
}

void hyacinth::load(const boost::filesystem::path& f, std::string& s) {
  boost::filesystem::ifstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  file.open(f, std::ios_base::binary);
  std::size_t size = static_cast<std::size_t>(boost::filesystem::file_size(f));
  s.resize(size, '\0');
  file.read(&s[0], size);
}

std::string hyacinth::execute(const std::vector<std::string> args) {
  const std::string command = boost::algorithm::join(args, " ");
  BOOST_LOG_TRIVIAL(debug) << "call: " << command;
  std::shared_ptr<FILE> pipe(popen(command.c_str(), "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("couldn't open pipe");
  };
  char buffer[128];
  std::stringstream ss;
  while (!feof(pipe.get())) {
    if (fgets(buffer, 128, pipe.get()) != nullptr) {
      ss << buffer;
    }
  }
  const auto out = ss.str();
  BOOST_LOG_TRIVIAL(debug) << out;
  return out;
}
