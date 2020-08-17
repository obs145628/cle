set(SRC
  lib/isa.cc
  lib/gop.cc
  lib/module.cc
  lib/module-load.cc
  lib/pp.cc

  main.cc
)
add_executable(procedure-placement ${SRC})
target_link_libraries(procedure-placement utils_cli utils_str)
