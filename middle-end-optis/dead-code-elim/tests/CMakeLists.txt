add_test(NAME ex1 COMMAND ${CMAKE_BINARY_DIR}/bin/dead-code-elim ${CMAKE_SOURCE_DIR}/examples/ex1.ll)
add_test(NAME ex2 COMMAND ${CMAKE_BINARY_DIR}/bin/dead-code-elim ${CMAKE_SOURCE_DIR}/examples/ex2.ll)
add_test(NAME ex3 COMMAND ${CMAKE_BINARY_DIR}/bin/dead-code-elim ${CMAKE_SOURCE_DIR}/examples/ex3.ll)
add_test(NAME ex4 COMMAND ${CMAKE_BINARY_DIR}/bin/dead-code-elim ${CMAKE_SOURCE_DIR}/examples/ex4.ll)
add_test(NAME ex5 COMMAND ${CMAKE_BINARY_DIR}/bin/dead-code-elim ${CMAKE_SOURCE_DIR}/examples/ex5.ll)
add_test(NAME ex6 COMMAND ${CMAKE_BINARY_DIR}/bin/dead-code-elim ${CMAKE_SOURCE_DIR}/examples/ex6.ll)
add_test(NAME ex7 COMMAND ${CMAKE_BINARY_DIR}/bin/dead-code-elim ${CMAKE_SOURCE_DIR}/examples/ex7.ll)
add_test(NAME ex8 COMMAND ${CMAKE_BINARY_DIR}/bin/dead-code-elim ${CMAKE_SOURCE_DIR}/examples/ex8.ll)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS dead-code-elim)
