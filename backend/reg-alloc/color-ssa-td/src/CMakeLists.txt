set(SRC

  isa/isa.cc
  isa/module.cc

  lib/allocator.cc
  lib/block-freq.cc
  lib/cfg.cc
  lib/live-now.cc
  lib/live-out.cc

  utils/digraph.cc

  main.cc
)
add_executable(ralloc-col-ssa-td ${SRC})
target_link_libraries(ralloc-col-ssa-td mdlogger gop10 utils_cli utils_str)
