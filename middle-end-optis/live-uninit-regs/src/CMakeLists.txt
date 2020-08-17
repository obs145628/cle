set(SRC
  lib/cfg.cc
  lib/digraph.cc
  lib/imodule.cc
  lib/liveout.cc
  lib/module.cc

  main.cc
)
add_executable(live-uninit-regs ${SRC})
target_link_libraries(live-uninit-regs utils_cli utils_str)
