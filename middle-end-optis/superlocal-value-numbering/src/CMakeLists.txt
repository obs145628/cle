set(SRC
  lib/bb.cc
  lib/cfg.cc
  lib/digraph.cc
  lib/slvn.cc
  lib/module.cc

  main.cc
)
add_executable(superlocal-value-numbering ${SRC})
target_link_libraries(superlocal-value-numbering utils_cli utils_str)
