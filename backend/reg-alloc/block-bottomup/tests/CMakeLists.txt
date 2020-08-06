

add_test(NAME ex1_4 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-bottomup ${CMAKE_SOURCE_DIR}/examples/ex1.ir 4)
add_test(NAME ex1_3 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-bottomup ${CMAKE_SOURCE_DIR}/examples/ex1.ir 3)
add_test(NAME matmul_8 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-bottomup ${CMAKE_SOURCE_DIR}/examples/matmul.ir 8)
add_test(NAME sum4_3 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-bottomup ${CMAKE_SOURCE_DIR}/examples/sum4.ir 3)
add_test(NAME sum4s_3 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-bottomup ${CMAKE_SOURCE_DIR}/examples/sum4s.ir 3)
add_test(NAME sum4s_2 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-bottomup ${CMAKE_SOURCE_DIR}/examples/sum4s.ir 2)
add_test(NAME sumsq4_4 COMMAND ${CMAKE_BINARY_DIR}/bin/ralloc-bottomup ${CMAKE_SOURCE_DIR}/examples/sumsq4.ir 4)


add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS ralloc-bottomup)
