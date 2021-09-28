cmake_minimum_required(VERSION 3.7)
project(hashtable_benchmark)

option(SANITIZE "Building the library with sanitizers." OFF)
set(CMAKE_CXX_STANDARD 17)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   add_compile_options (-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   add_compile_options (-fcolor-diagnostics)
endif ()


if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()
if (SANITIZE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -Wpedantic -Wno-gcc-compat")
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
set(BENCHMARK_ENABLE_TESTING OFF)
message(STATUS "gtest tests are off")
add_subdirectory(benchmark)
add_subdirectory(abseil-cpp)
add_executable(${PROJECT_NAME} benchmark.cpp)
target_link_libraries(
  ${PROJECT_NAME}
  absl::base
  absl::flat_hash_map
  absl::flat_hash_set
  absl::hash
  absl::random_distributions
  absl::random_random
  absl::synchronization
  benchmark::benchmark)
