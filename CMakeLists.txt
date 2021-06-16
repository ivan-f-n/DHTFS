cmake_minimum_required(VERSION 3.14)
project(my_project)

# GoogleTest requires at least C++11
set(CMAKE_CXX_STANDARD 14)

include(FindPkgConfig)
pkg_search_module(FUSE REQUIRED fuse3)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/53495a2a7d6ba7e0691a7f3602e9a5324bba6e45.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
  tests
  test/tests.cpp
)
target_include_directories(tests SYSTEM PUBLIC ${FUSE_INCLUDE_DIRS})
target_link_libraries(
  tests
  gtest_main
  gmock
  opendht
  gnutls
  ${FUSE_LIBRARIES}
)

include(GoogleTest)
gtest_discover_tests(tests)

