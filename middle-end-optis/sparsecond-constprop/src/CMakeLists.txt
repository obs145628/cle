set(SRC
  isa/isa.cc
  
  lib/cfg.cc
  lib/digraph.cc
  lib/digraph-order.cc
  lib/scc.cc
  lib/loader.cc
  lib/module.cc
  lib/names-table.cc
  lib/idom.cc
  lib/value.cc

  main.cc
)
add_executable(sparsecond-constprop ${SRC})
target_link_libraries(sparsecond-constprop gop10 utils_cli utils_str)
