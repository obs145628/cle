set(SRC
  isa/isa.cc
  
  lib/cfg.cc
  lib/digraph.cc
  lib/digraph-order.cc
  lib/loader.cc
  lib/module.cc
  lib/names-table.cc
  lib/ipcp.cc
  lib/value.cc

  main.cc
)
add_executable(interproc-constprop ${SRC})
target_link_libraries(interproc-constprop gop10 utils_cli utils_str)
