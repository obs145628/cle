set(SRC

  isa/isa.cc
  isa/module.cc

  lib/allocator.cc

  utils/digraph.cc

  main.cc
)
add_executable(ralloc-block-naive ${SRC})
target_link_libraries(ralloc-block-naive mdlogger gop10 utils_cli utils_str)
