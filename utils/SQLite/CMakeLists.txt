PROJECT(SQLite)

INCLUDE_DIRECTORIES(
        ${SQLite_SOURCE_DIR}
)

#add_definitions(-DSQLITE_EXPORTS)
#include(GenerateExportHeader)

if(WIN32)
    ADD_LIBRARY(SQLite STATIC sqlite3.c)
else()
    ADD_LIBRARY(SQLite SHARED sqlite3.c)
endif()

#generate_export_header(SQLite)

#install(TARGETS SQLite LIBRARY DESTINATION
#        ${CMAKE_INSTALL_PREFIX}/lib)


install(FILES sqlite3.h sqlite3ext.h DESTINATION include)
if(WIN32)
        install(TARGETS SQLite
                ARCHIVE DESTINATION lib
                #RUNTIME DESTINATION lib
        )
else()
        install(TARGETS SQLite LIBRARY DESTINATION lib)
endif()
