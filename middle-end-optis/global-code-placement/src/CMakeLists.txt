set(SRC
  lib/cfg.cc
  lib/digraph.cc
  lib/imodule.cc
  lib/gcp.cc
  lib/module.cc

  main.cc
)
add_executable(global-code-placement ${SRC})
target_link_libraries(global-code-placement utils_cli utils_str)
