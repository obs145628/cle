set(SRC
  isa/isa.cc
  
  lib/cfg.cc
  lib/digraph.cc
  lib/digraph-order.cc
  lib/loader.cc
  lib/module.cc
  lib/names-table.cc
  lib/sscp.cc
  lib/value.cc

  main.cc
)
add_executable(sparse-simple-constprop ${SRC})
target_link_libraries(sparse-simple-constprop gop10 utils_cli utils_str)
