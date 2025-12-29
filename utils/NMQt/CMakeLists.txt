PROJECT(NMQt)

set(NMQSQLite_SRC_DIR ${NMQt_SOURCE_DIR}/drivers/sqlite
    CACHE FILEPATH "Path to LUMASS internal copy of Qt's QSQLiteDriver code")
mark_as_advanced(FORCE NMQSQLite_SRC_DIR)

INCLUDE_DIRECTORIES(
    ${NMQt_SOURCE_DIR}
    ${NMQt_SOURCE_DIR}/drivers
    ${NMQt_SOURCE_DIR}/drivers/sqlite
    ${shared_SOURCE_DIR}
    ${QT5_INCLUDE_DIRS}
        #${QT5CORE_PRIVATE_DIR}
        #${QT5CORE_PRIVATE_DIR}/..
    ${SQLite_SOURCE_DIR}
    ${SPATIALITE_INCLUDE_DIR}
        #${QT5SQL_INCLUDE_DIR}
        #${QT5WIDGETS_INCLUDE_DIR}
        #${NMQT5SQL_PRIVATE_DIR}
        #${NMQT5SQL_PRIVATE_DIR}/..
    ${MPI_CXX_INCLUDE_DIRS}

        # dirty hacker...
        #C:/src/qt-everywhere-src-5.15.17/qtbase/include/QtCore/5.15.17/QtCore/private
        #C:/src/qt-everywhere-src-5.15.17/qtbase/include/QtCore/5.15.17/QtCore
        #C:/opt/qt/5.15.17/qtbase/include/QtCore/5.15.17/QtCore/private
        #C:/src/qt-everywhere-src-5.15.17/qtbase/include/QtCore/5.15.17
        #C:/src/qt-everywhere-src-5.15.17/qtbase/include/QtSql/5.15.17
        #C:/opt/qt/5.15.17/qtbase/include/QtSql/5.15.17
        #C:/src/qt-everywhere-src-5.15.17/qtbase/src/sql/kernel
        #C:/src/qt-everywhere-src-5.15.17/qtbase/src/corelib/global
        #C:/src/qt-everywhere-src-5.15.17/qtbase/src/corelib
        #C:/src/qt-everywhere-src-5.15.17/qtbase/src
        #C:/src/qt-everywhere-src-5.15.17/qtbase/
)


LINK_DIRECTORIES(
        #${QT5_SQLDRIVERS_DIR}
        #${QT5_PREFIX_PATH}/../plugins/sqldrivers
        #${QT5_LINK_DIRS}
        #${QT5_SQLDRIVERS_DIR}
    ${NMQt_BINARY_DIR}
    ${SQLite_BINARY_DIR}/lib
    ${SPATIALITE_LIB_DIR}
    ${SPATIALITE_LIBLIB_DIR}
)

SET(CMAKE_AUTOMOC YES)

file(GLOB NMQT_SRC
    ${NMQSQLite_SRC_DIR}/*.h
    ${NMQSQLite_SRC_DIR}/*.cpp
    ${NMQt_SOURCE_DIR}/*.h
    ${NMQt_SOURCE_DIR}/*.cpp
)

ADD_LIBRARY(NMQt STATIC ${NMQT_SRC})

if (WIN32)
    target_link_libraries(NMQt
        Qt5::Core Qt5::Sql Qt5::Gui
        SQLite
        spatialite
        geos_c
        freexl_i
        libxml2
        zlib 
        iconv
        librttopo
        proj
    )

    install (TARGETS NMQt
             LIBRARY DESTINATION lib
             RUNTIME DESTINATION bin
     )
else()
    target_link_libraries(NMQt
        Qt5::Core Qt5::Sql Qt5::Gui
        SQLite
        spatialite
    )

    install(TARGETS NMQt LIBRARY DESTINATION lib)
endif()



