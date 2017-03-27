set(SOURCES cqlite_tests.c test_database.c)

add_executable(test_cqlite ${SOURCES})

target_link_libraries(test_cqlite cqlite unity sqlite3)

add_test(cqlite test_cqlite)