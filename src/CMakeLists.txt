set(SOURCES cqlite.c)
set(HEADERS cqlite.h)

add_library(cqlite ${SOURCES} ${HEADERS})

target_include_directories(cqlite PUBLIC ${CMAKE_CURRENT_LIST_DIR})