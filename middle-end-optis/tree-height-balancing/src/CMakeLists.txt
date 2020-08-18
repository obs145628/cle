include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs core irreader)

set(SRC
  lib/thb.cc
  main.cc
)
add_executable(tree-height-balancing ${SRC})
target_link_libraries(tree-height-balancing ${llvm_libs} utils_cli utils_str)
