set(SRC

  isa/isa.cc
  isa/module.cc

  lib/allocator.cc

  utils/digraph.cc

  main.cc
)
add_executable(ralloc-bottomup ${SRC})
target_link_libraries(ralloc-bottomup logia gop10 utils_cli utils_str)
