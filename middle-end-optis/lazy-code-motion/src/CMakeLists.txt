set(SRC
  lib/cfg.cc
  lib/digraph.cc
  lib/imodule.cc
  lib/lcm.cc
  lib/module.cc

  main.cc
)
add_executable(lazy-code-motion ${SRC})
target_link_libraries(lazy-code-motion utils_cli utils_str)
