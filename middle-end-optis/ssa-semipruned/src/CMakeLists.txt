set(SRC
  lib/cfg.cc
  lib/digraph.cc
  lib/digraph-order.cc
  lib/dom.cc
  lib/dom-frontier.cc
  lib/dom-tree.cc
  lib/isa.cc
  lib/gop.cc
  lib/module.cc
  lib/module-load.cc
  lib/ssa.cc

  main.cc
)
add_executable(ssa-semipruned ${SRC})
target_link_libraries(ssa-semipruned utils_cli utils_str)
