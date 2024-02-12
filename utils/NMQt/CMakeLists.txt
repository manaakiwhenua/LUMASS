PROJECT(NMQt)

set(NMQSQLite_SRC_DIR ${NMQt_SOURCE_DIR}/drivers/sqlite
    CACHE FILEPATH "Path to LUMASS internal copy of Qt's QSQLiteDriver code")
mark_as_advanced(FORCE NMQSQLite_SRC_DIR)

INCLUDE_DIRECTORIES(
    ${shared_SOURCE_DIR}
    ${QT5_INCLUDE_DIRS}
    ${QT5CORE_PRIVATE_DIR}
    ${QT5CORE_PRIVATE_DIR}/..
    ${QT5SQL_INCLUDE_DIR}
    #${QT5WIDGETS_INCLUDE_DIR}
    ${NMQT5SQL_PRIVATE_DIR}
    ${NMQT5SQL_PRIVATE_DIR}/..
    ${MPI_CXX_INCLUDE_DIRS}
)


LINK_DIRECTORIES(
    ${QT5_SQLDRIVERS_DIR}
    ${QT5_PREFIX_PATH}/../plugins/sqldrivers
    ${QT5_LINK_DIRS}
    ${QT5_SQLDRIVERS_DIR}
    ${NMQt_BINARY_DIR}
    ${SQLite_BINARY_DIR}/lib
    ${SPATIALITE_LIB_DIR}
    ${SPATIALITE_LIBLIB_DIR}
)

file(GLOB NMQT_SRC
    ${NMQSQLite_SRC_DIR}/*.cpp
    ${NMQt_SOURCE_DIR}/*.cpp
)


SET(CMAKE_AUTOMOC YES)

ADD_LIBRARY(NMQt SHARED ${NMQT_SRC})
target_link_libraries(NMQt
    Qt5::Core Qt5::Sql Qt5::Gui
    #qsqlite
    SQLite
    spatialite
)

if (WIN32)
    install (TARGETS NMQt
             LIBRARY DESTINATION lib
             RUNTIME DESTINATION lib
     )
else()
    install(TARGETS NMQt LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif()



