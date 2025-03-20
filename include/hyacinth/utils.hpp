#pragma once

#include <chrono>
#include <ctime>
#include <string>

#include <boost/filesystem.hpp>

#include <nlohmann/json.hpp>

namespace hyacinth {
std::string timestamp(std::time_t it);
inline std::string timestamp() {
  const auto now = std::chrono::system_clock::now();
  return timestamp(std::chrono::system_clock::to_time_t(now));
}
void render(const boost::filesystem::path& file, const std::string& tpl,
            const nlohmann::json& data);
void load(const boost::filesystem::path& f, std::string& s);
std::string execute(const std::vector<std::string> args);
}  // namespace hyacinth
