set(SRC
  module.cc
)
add_library(gop10 ${SRC})
target_link_libraries(gop10 utils_cli utils_str)
