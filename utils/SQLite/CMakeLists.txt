PROJECT(SQLite)

INCLUDE_DIRECTORIES(
        ${SQLite_SOURCE_DIR}
)

LINK_DIRECTORIES(
    ${SQLite_BRINARY_DIR}/lib
)

#add_definitions(-DSQLITE_CORE)
#ADD_LIBRARY(SQLiteExtFunc STATIC sqlite3extfunc.c)
add_definitions(-DSQLITE_THREADSAFE=1)
add_definitions(-DSQLITE_ENABLE_RTREE=1)

if(WIN32)
    ADD_LIBRARY(SQLite STATIC sqlite3.c)
else()
    #ADD_LIBRARY(SQLiteExtFunc STATIC sqlite3extfunc.c)
    ADD_LIBRARY(SQLite SHARED sqlite3.c)
    target_link_libraries(SQLite pthread dl)
endif()


#install(FILES sqlite3.h sqlite3ext.h DESTINATION include)
if(WIN32)
#        install(TARGETS SQLiteExtFunc
#                LIBRARY DESTINATION lib
#                RUNTIME DESTINATION lib
#                ARCHIVE DESTINATION lib
#        )

#        install(TARGETS SQLite
#                LIBRARY DESTINATION lib
#                RUNTIME DESTINATION lib
#        )


        install(TARGETS SQLite
                ARCHIVE DESTINATION lib
        )
else()
        #install(TARGETS SQLiteExtFunc ARCHIVE DESTINATION lib)
        install(TARGETS SQLite LIBRARY DESTINATION lib)
endif()
