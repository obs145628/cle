cmake_minimum_required(VERSION 3.0)

execute_process(COMMAND git rev-parse --show-toplevel OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE GIT_ROOT)

include_directories(SYSTEM ${GIT_ROOT}/utils/libcpp_utils/include)

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -Werror -O0 -g3")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(include)

add_subdirectory(src)

add_subdirectory(tests)
