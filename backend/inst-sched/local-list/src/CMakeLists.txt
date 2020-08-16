set(SRC
  main.cc

  isa/isa.cc
  isa/module.cc

  lib/dep.cc
  lib/renamer.cc
  lib/sched.cc

  utils/digraph.cc
)
add_executable(isched-local-list ${SRC})
target_link_libraries(isched-local-list logia gop10 utils_cli utils_str)
