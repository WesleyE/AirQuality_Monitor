cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(ObserveTests
  LANGUAGES CXX
)

# ---- Options ----

option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)
option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

CPMAddPackage(
  NAME doctest
  GITHUB_REPOSITORY onqtam/doctest
  GIT_TAG 2.3.7
)

if (TEST_INSTALLED_VERSION)
  find_package(Observe REQUIRED)
else()
  CPMAddPackage(
    NAME Observe
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..
  )
endif()

CPMAddPackage(
  NAME Format.cmake
  GITHUB_REPOSITORY TheLartians/Format.cmake
  VERSION 1.3
)

# ---- Create binary ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(ObserveTests ${sources})
target_link_libraries(ObserveTests doctest Observe)

set_target_properties(ObserveTests PROPERTIES CXX_STANDARD 17)

# enable compiler warnings
if (NOT TEST_INSTALLED_VERSION)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(Observe INTERFACE -Wall -pedantic -Wextra -Werror)
  elseif(MSVC)
    target_compile_options(Observe INTERFACE /W4 /WX)
  endif()
endif()

# ---- Add ObserveTests ----

ENABLE_TESTING()

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(ObserveTests)

# ---- code coverage ----

if (ENABLE_TEST_COVERAGE)
  target_compile_options(Observe INTERFACE -O0 -g -fprofile-arcs -ftest-coverage)
  target_link_options(Observe INTERFACE -fprofile-arcs -ftest-coverage)
endif()
