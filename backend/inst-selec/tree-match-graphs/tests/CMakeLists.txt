add_test(NAME add_mem_aa64 COMMAND ${CMAKE_BINARY_DIR}/bin/iselec-tree-match-graphs aa64 ${CMAKE_SOURCE_DIR}/examples/add_mem.lir)
add_test(NAME add_mem_x64 COMMAND ${CMAKE_BINARY_DIR}/bin/iselec-tree-match-graphs x64 ${CMAKE_SOURCE_DIR}/examples/add_mem.lir)
add_test(NAME matvecmul_aa64 COMMAND ${CMAKE_BINARY_DIR}/bin/iselec-tree-match-graphs aa64 ${CMAKE_SOURCE_DIR}/examples/matvecmul.lir)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS iselec-tree-match-graphs)
