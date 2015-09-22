PROJECT(SQLite)

INCLUDE_DIRECTORIES(
        ${SQLite_SOURCE_DIR}
)

ADD_LIBRARY(SQLite SHARED sqlite3.c)


install(TARGETS SQLite LIBRARY DESTINATION
        ${CMAKE_INSTALL_PREFIX}/lib)


install(FILES sqlite3.h sqlite3ext.h DESTINATION include)
if(WIN32)
        install(TARGETS SQLite
                LIBRARY DESTINATION lib
                RUNTIME DESTINATION lib
        )
else()
        install(TARGETS SQLite LIBRARY DESTINATION lib)
endif()
