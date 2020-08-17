set(SRC
  lib/lvn.cc
  lib/lvn_ext.cc
  lib/module.cc

  main.cc
)
add_executable(local-value-numbering ${SRC})
target_link_libraries(local-value-numbering utils_cli utils_str)
