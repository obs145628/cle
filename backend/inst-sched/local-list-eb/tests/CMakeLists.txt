

add_test(NAME dist COMMAND ${CMAKE_BINARY_DIR}/bin/isched-local-list-eb ${CMAKE_SOURCE_DIR}/examples/dist.ir)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS isched-local-list-eb)
