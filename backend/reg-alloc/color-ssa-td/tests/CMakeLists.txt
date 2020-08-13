

add_test(NAME block1_8 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-col-ssa-td mdlogger ${CMAKE_SOURCE_DIR}/examples/block1.ir 8)
add_test(NAME block2_8 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-col-ssa-td mdlogger ${CMAKE_SOURCE_DIR}/examples/block2.ir 8)
add_test(NAME ex1_8 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-col-ssa-td mdlogger ${CMAKE_SOURCE_DIR}/examples/ex1.ir 8)


add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS ralloc-col-ssa-td)
