#include <cstdlib>

#include <boost/program_options.hpp>

#include "hyacinth/application.hpp"
#include "hyacinth/database.hpp"

int main(int argc, char** argv) {
  BOOST_LOG_TRIVIAL(debug) << "A trace severity message";
  return EXIT_SUCCESS;
}
