

add_test(NAME matmul COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-block-naive ${CMAKE_SOURCE_DIR}/examples/matmul.ir)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS ralloc-block-naive)
