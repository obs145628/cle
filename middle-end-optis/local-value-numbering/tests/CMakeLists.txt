add_test(NAME ex1 COMMAND ${CMAKE_BINARY_DIR}/bin/local-value-numbering ${CMAKE_SOURCE_DIR}/examples/ex1.ir)
add_test(NAME ex2 COMMAND ${CMAKE_BINARY_DIR}/bin/local-value-numbering ${CMAKE_SOURCE_DIR}/examples/ex2.ir)
add_test(NAME ex3 COMMAND ${CMAKE_BINARY_DIR}/bin/local-value-numbering ${CMAKE_SOURCE_DIR}/examples/ex3.ir)
add_test(NAME ex4 COMMAND ${CMAKE_BINARY_DIR}/bin/local-value-numbering ${CMAKE_SOURCE_DIR}/examples/ex4.ir --ext)
add_test(NAME ex5 COMMAND ${CMAKE_BINARY_DIR}/bin/local-value-numbering ${CMAKE_SOURCE_DIR}/examples/ex5.ir --ext)
add_test(NAME ex6 COMMAND ${CMAKE_BINARY_DIR}/bin/local-value-numbering ${CMAKE_SOURCE_DIR}/examples/ex6.ir --ext)


add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS local-value-numbering)
