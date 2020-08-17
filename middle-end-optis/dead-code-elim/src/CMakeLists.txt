llvm_map_components_to_libnames(llvm_libs core irreader)

set(SRC
  lib/cfg.cc
  lib/digraph.cc
  lib/digraph-order.cc
  lib/dom-tree.cc
  lib/dom-frontier.cc
  lib/dce.cc
  lib/dcfe.cc
  lib/unreachable.cc

  main.cc
)
add_executable(dead-code-elim ${SRC})
target_link_libraries(dead-code-elim ${llvm_libs} utils_cli utils_str)
