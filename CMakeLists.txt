cmake_minimum_required(VERSION 3.22)
include(FetchContent)

project(
  gourd
  VERSION 2025.3.13
  DESCRIPTION "A database migration tool that help to keep your database schema."
  LANGUAGES CXX
)

# ---------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-s -Wl,--build-id=sha1")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --------------------------------------------------------

execute_process(COMMAND git describe --tags --always --dirty
    OUTPUT_VARIABLE GIT_REV
    ERROR_QUIET
)
string(STRIP "${GIT_REV}" GIT_REV)
string(TIMESTAMP BUILD_TIME UTC)
configure_file(include/hyacinth/version.h.in ${PROJECT_SOURCE_DIR}/include/hyacinth/version.hpp)

# ---------------------------------------------------------

find_package(Boost REQUIRED COMPONENTS log program_options)
find_package(PostgreSQL REQUIRED)
find_package(unofficial-libmariadb CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

# ---------------------------------------------------------

file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(hyacinth ${SOURCE_LIST})
set_target_properties(hyacinth PROPERTIES LINK_FLAGS_RELEASE -s)

target_link_libraries(hyacinth
  Boost::log Boost::program_options
  PostgreSQL::PostgreSQL unofficial::libmariadb unofficial::sqlite3::sqlite3
)

target_include_directories(hyacinth PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

