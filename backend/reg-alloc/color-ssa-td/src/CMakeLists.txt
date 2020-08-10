set(SRC

  isa/isa.cc
  isa/module.cc

  lib/allocator.cc

  utils/digraph.cc

  main.cc
)
add_executable(ralloc-col-ssa-td ${SRC})
target_link_libraries(ralloc-col-ssa-td mdlogger gop10 utils_cli utils_str)
