add_test(NAME ex1 COMMAND ${CMAKE_BINARY_DIR}/bin/ssa-semipruned ${CMAKE_SOURCE_DIR}/examples/ex1.ir)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS ssa-semipruned)
