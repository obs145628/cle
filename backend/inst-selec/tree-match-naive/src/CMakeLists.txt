set(SRC
  isa/isa.cc

  lib/matcher.cc
  lib/rules.cc
  lib/tree-block.cc

  utils/digraph.cc

  main.cc
)
add_executable(iselec-tree-match-naive ${SRC})
target_link_libraries(iselec-tree-match-naive logia gop10 utils_cli utils_str)
