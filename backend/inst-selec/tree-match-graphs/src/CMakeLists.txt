set(SRC
  isa/isa.cc
  
  lib/block-graph.cc
  lib/cfg.cc
  lib/context.cc
  lib/context-aa64.cc
  lib/context-x64.cc
  lib/live-out.cc
  lib/module.cc
  lib/module-pass.cc
  lib/matcher.cc
  lib/rules.cc

  utils/digraph.cc

  main.cc
)
add_executable(iselec-tree-match-graphs ${SRC})
target_link_libraries(iselec-tree-match-graphs logia gop10 utils_cli utils_str)
