

add_test(NAME block1_4 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-col-ssa-bu mdlogger ${CMAKE_SOURCE_DIR}/examples/block1.ir 4)
add_test(NAME block2_4 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-col-ssa-bu mdlogger ${CMAKE_SOURCE_DIR}/examples/block2.ir 4)
add_test(NAME ex1_4 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-col-ssa-bu mdlogger ${CMAKE_SOURCE_DIR}/examples/ex1.ir 4)
add_test(NAME matvecmul_16 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-col-ssa-bu mdlogger ${CMAKE_SOURCE_DIR}/examples/matvecmul.ir 16)
add_test(NAME block1_3 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-col-ssa-bu mdlogger ${CMAKE_SOURCE_DIR}/examples/block1.ir 3)
add_test(NAME block2_3 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-col-ssa-bu mdlogger ${CMAKE_SOURCE_DIR}/examples/block2.ir 3)
add_test(NAME matvecmul_8 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-col-ssa-bu mdlogger ${CMAKE_SOURCE_DIR}/examples/matvecmul.ir 8)
add_test(NAME coalesce_2 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-col-ssa-bu mdlogger ${CMAKE_SOURCE_DIR}/examples/coalesce.ir 2)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS ralloc-col-ssa-bu)
