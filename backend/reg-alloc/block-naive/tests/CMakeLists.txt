

add_test(NAME ex1_8 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-block-naive ${CMAKE_SOURCE_DIR}/examples/ex1.ir 8)
add_test(NAME ex1_16 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-block-naive ${CMAKE_SOURCE_DIR}/examples/ex1.ir 16)
add_test(NAME matmul_8 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-block-naive ${CMAKE_SOURCE_DIR}/examples/matmul.ir 8)


add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS ralloc-block-naive)
