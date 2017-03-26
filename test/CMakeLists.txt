add_executable(test_cqlite cqlite_tests.c)

target_link_libraries(test_cqlite cqlite unity)

add_test(cqlite test_cqlite)