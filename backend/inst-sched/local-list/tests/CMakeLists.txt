

add_test(NAME ex1 COMMAND ${CMAKE_BINARY_DIR}/bin/isched-local-list ${CMAKE_SOURCE_DIR}/examples/ex1.ir)
add_test(NAME ex2 COMMAND ${CMAKE_BINARY_DIR}/bin/isched-local-list ${CMAKE_SOURCE_DIR}/examples/ex2.ir)


add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS isched-local-list)
