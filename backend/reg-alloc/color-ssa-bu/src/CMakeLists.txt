set(SRC

  isa/isa.cc
  isa/module.cc

  lib/allocator.cc
  lib/block-freq.cc
  lib/cfg.cc
  lib/coalescing.cc
  lib/interference-graph.cc
  lib/live-now.cc
  lib/live-out.cc
  lib/spill-cost.cc

  utils/digraph.cc
  utils/graph.cc

  main.cc
)
add_executable(ralloc-col-ssa-bu ${SRC})
target_link_libraries(ralloc-col-ssa-bu logia gop10 utils_cli utils_str)
