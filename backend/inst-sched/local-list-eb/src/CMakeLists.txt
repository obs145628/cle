set(SRC
  main.cc
  
  isa/isa.cc
  isa/module.cc

  lib/cfg.cc
  lib/dep.cc
  lib/eb-paths.cc
  lib/live-out.cc
  lib/renamer.cc
  lib/sched.cc

  utils/digraph.cc
)
add_executable(isched-local-list-eb ${SRC})
target_link_libraries(isched-local-list-eb gop10 logia utils_cli utils_str)
