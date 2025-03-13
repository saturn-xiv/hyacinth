#include <cstdlib>

#include "hyacinth/application.hpp"

int main(int argc, char** argv) {
  hyacinth::Application app;
  app.launch(argc, argv);
  return EXIT_SUCCESS;
}
