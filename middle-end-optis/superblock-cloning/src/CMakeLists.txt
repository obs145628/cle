set(SRC
  isa/isa.cc
  
  lib/cfg.cc
  lib/cloner.cc
  lib/digraph.cc
  lib/digraph-order.cc
  lib/sbc.cc
  lib/loader.cc
  lib/module.cc
  lib/names-table.cc
  lib/idom.cc
  lib/value.cc

  main.cc
)
add_executable(superblock-cloning ${SRC})
target_link_libraries(superblock-cloning gop10 utils_cli utils_str)
