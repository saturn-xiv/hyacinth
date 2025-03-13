#include <cstdlib>

#include "hyacinth/application.hpp"

#include <boost/exception/diagnostic_information.hpp>

int main(int argc, char** argv) {
  hyacinth::Application app;
  try {
    app.launch(argc, argv);
    return EXIT_SUCCESS;
  } catch (...) {
    BOOST_LOG_TRIVIAL(error)
        << boost::current_exception_diagnostic_information();
  }
  return EXIT_FAILURE;
}
