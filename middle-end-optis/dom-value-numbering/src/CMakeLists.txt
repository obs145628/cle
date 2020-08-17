set(SRC
  isa/isa.cc
  
  lib/cfg.cc
  lib/digraph.cc
  lib/digraph-order.cc
  lib/dvnt.cc
  lib/loader.cc
  lib/module.cc
  lib/names-table.cc
  lib/idom.cc
  lib/value.cc

  main.cc
)
add_executable(optime-dom-value-numbering ${SRC})
target_link_libraries(optime-dom-value-numbering gop10 utils_cli utils_str)
#target_link_libraries(optime-dom-value-numbering simplevm10 gop10)
