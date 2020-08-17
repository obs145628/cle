set(SRC
  isa/isa.cc
  
  lib/cfg.cc
  lib/digraph.cc
  lib/digraph-order.cc
  lib/loader.cc
  lib/module.cc
  lib/names-table.cc
  lib/idom.cc
  lib/value.cc

  main.cc
)
add_executable(idom ${SRC})
target_link_libraries(idom gop10 utils_cli utils_str)
