llvm_map_components_to_libnames(llvm_libs core irreader)

set(SRC
  lib/cfg.cc
  lib/digraph.cc
  lib/digraph-order.cc
  lib/dom.cc
  
  main.cc
)
add_executable(dominance ${SRC})
target_link_libraries(dominance ${llvm_libs} utils_cli utils_str)
