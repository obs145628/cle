set(SRC
  lib/bbmerge.cc
  lib/cfg.cc
  lib/copy.cc
  lib/digraph.cc
  lib/inline.cc
  lib/isa.cc
  lib/gop.cc
  lib/module.cc
  lib/module-load.cc
  lib/pass_inline.cc

  main.cc
)
add_executable(fun-inliner ${SRC})
target_link_libraries(fun-inliner utils_cli utils_str)
