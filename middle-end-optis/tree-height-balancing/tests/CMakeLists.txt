add_test(NAME ex1 COMMAND ${CMAKE_BINARY_DIR}/bin/tree-height-balancing ${CMAKE_SOURCE_DIR}/examples/ex1.ll)
add_test(NAME ex2 COMMAND ${CMAKE_BINARY_DIR}/bin/tree-height-balancing ${CMAKE_SOURCE_DIR}/examples/ex2.ll)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS tree-height-balancing)
