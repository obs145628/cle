set(SRC
  isa/isa.cc
  
  lib/cfg.cc
  lib/critical.cc
  lib/digraph.cc
  lib/digraph-order.cc
  lib/loader.cc
  lib/module.cc
  lib/names-table.cc
  lib/unssa.cc
  lib/value.cc

  main.cc
)

add_executable(unssa ${SRC})
target_link_libraries(unssa gop10 utils_cli utils_str)
