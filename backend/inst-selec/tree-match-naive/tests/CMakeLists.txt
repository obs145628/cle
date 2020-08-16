add_test(NAME tree1_args_aa64 COMMAND ${CMAKE_BINARY_DIR}/bin/iselec-tree-match-naive aa64 ${CMAKE_SOURCE_DIR}/examples/tree1_args.lir)
add_test(NAME tree1_args_x64 COMMAND ${CMAKE_BINARY_DIR}/bin/iselec-tree-match-naive x64 ${CMAKE_SOURCE_DIR}/examples/tree1_args.lir)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS iselec-tree-match-naive)
