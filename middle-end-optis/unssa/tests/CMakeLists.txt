add_test(NAME call_add COMMAND ${CMAKE_BINARY_DIR}/bin/unssa ${CMAKE_SOURCE_DIR}/examples/call_add.ir)
add_test(NAME fact_iter_ssa COMMAND ${CMAKE_BINARY_DIR}/bin/unssa ${CMAKE_SOURCE_DIR}/examples/fact_iter_ssa.ir)
add_test(NAME fact_rec_ssa COMMAND ${CMAKE_BINARY_DIR}/bin/unssa ${CMAKE_SOURCE_DIR}/examples/fact_rec_ssa.ir)
add_test(NAME lost_copy COMMAND ${CMAKE_BINARY_DIR}/bin/unssa ${CMAKE_SOURCE_DIR}/examples/lost_copy.ir)
add_test(NAME swap1 COMMAND ${CMAKE_BINARY_DIR}/bin/unssa ${CMAKE_SOURCE_DIR}/examples/swap1.ir)
add_test(NAME swap2 COMMAND ${CMAKE_BINARY_DIR}/bin/unssa ${CMAKE_SOURCE_DIR}/examples/swap2.ir)


add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS unssa)
