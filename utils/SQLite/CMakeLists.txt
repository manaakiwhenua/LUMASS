PROJECT(SQLite)

INCLUDE_DIRECTORIES(
        ${SQLite_SOURCE_DIR}
)

#add_definitions(-DSQLITE_EXPORTS)
#include(GenerateExportHeader)

ADD_LIBRARY(SQLite STATIC sqlite3.c)

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
        install(TARGETS SQLite ARCHIVE DESTINATION lib)
endif()
