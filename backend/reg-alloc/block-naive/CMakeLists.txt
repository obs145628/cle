cmake_minimum_required(VERSION 3.0)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS "-std=c99 -Wall -Wextra -Werror -O0 -g3")

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -Werror -O0 -g3")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_definitions(-DCMAKE_SRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

execute_process(COMMAND git rev-parse --show-toplevel OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE GIT_ROOT)
set(GOP10_INCLUDE_DIRS ${GIT_ROOT}/utils/libcpp_gop10/include)
set(GOP10_LIBRARY_DIR ${GIT_ROOT}/utils/libcpp_gop10/_build/lib)
set(MDLOGGER_INCLUDE_DIRS ${GIT_ROOT}/utils/mdlogger/libcpp/include)
set(MDLOGGER_LIBRARY_DIR ${GIT_ROOT}/utils/mdlogger/libcpp/_build/lib)
set(UTILS_INCLUDE_DIRS ${GIT_ROOT}/utils/libcpp_utils/include)
set(UTILS_LIBRARY_DIR ${GIT_ROOT}/utils/libcpp_utils/_build/lib)

include_directories(SYSTEM ${GOP10_INCLUDE_DIRS})
link_directories(${GOP10_LIBRARY_DIR})
include_directories(SYSTEM ${MDLOGGER_INCLUDE_DIRS})
link_directories(${MDLOGGER_LIBRARY_DIR})
include_directories(SYSTEM ${UTILS_INCLUDE_DIRS})
link_directories(${UTILS_LIBRARY_DIR})

enable_testing()

add_subdirectory(src)

add_subdirectory(tests)
