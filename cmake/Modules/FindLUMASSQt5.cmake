# *******************************************************
# FindLUMASSQt5.cmake
# based on cmake 2.8.9
#
# finds main installation path of custom installed Qt5 (Qt-Project webiste download)
# ****************************************************

# CUSTQT5_FOUND

# ================================================
# define library names we're going to look for later
# ================================================

if(WIN32)
	set(QT5_CORE_LIB "lib/Qt5Core.lib")
    set(QT5SQLite_PLUGIN_LIBRARY "qsqlite.lib")
	set(QT5_PATHS
		C:/qt
		C:/
		C:/Qt
		C:/opt
		C:/opt/qt
		C:/opt/Qt
	)
    set(QT5_PATH_SUFFIXES
            Qt5.3.1/qtbase
            Qt5.3.1/5.3/qtbase
            Qt5.5.0/5.5/qtbase
            Qt5.5.0/qtbase
            5.3.1/qtbase
            5.4.2/qtbase
            5.4.2/5.4/qtbase
            5.3.1/5.3/qtbase
            5.5.0/5.5/qtbase
            5.5.0/qtbase
            5.5.1/qtbase
            5.6.0/qtbase
            5.6.1/qtbase
            5.6.1-1/qtbase
            5.6.2/qtbase
            5.7.0/qtbase
            5.7.1/qtbase
            5.8.0/qtbase
    )
else()
    set(QT5_CORE_LIB "libQt5Core.so")
    set(QT5SQLite_PLUGIN_LIBRARY "libqsqlite.so")
	set(QT5_PATHS
		/opt
		/opt/qt
                /opt/Qt
		/usr
		/usr/qt
		/usr/local
                /usr/lib
                /usr/lib/x86_64-linux-gnu
	)
    set(QT5_PATH_SUFFIXES
            Qt5.0.0/5.0.0/gcc
            Qt5.0.0/5.0.0/gcc_64
            Qt5.0.1/5.0.1/gcc
            Qt5.0.1/5.0.1/gcc_64
            Qt5.0.2/5.0.2/gcc
            Qt5.0.2/5.0.2/gcc_64
            Qt5.1.0/5.1.0/gcc
            Qt5.1.0/5.1.0/gcc_64
            Qt5.1.1/5.1.1/gcc
            Qt5.1.1/5.1.1/gcc_64
            Qt5.2.0/5.2.0/gcc
            Qt5.2.0/5.2.0/gcc_64
            Qt5.2.1/5.2.1/gcc
            Qt5.2.1/5.2.1/gcc_64
            Qt5.3.0/5.3.0/gcc
            Qt5.3.0/5.3.0/gcc_64
            Qt5.3.1/5.3.1/gcc
            Qt5.3.1/5.3.1/gcc_64
            Qt5.4.2/5.4.2/gcc
            Qt5.4.2/5.4.2/gcc_64
            Qt5.5.0/5.5.0/gcc
            Qt5.5.0/5.5.0/gcc_64
            Qt5.5.1/5.5.1/gcc
            Qt5.5.1/5.5.1/gcc_64
            Qt5.6.0/5.6.0/gcc
            Qt5.6.0/5.6.0/gcc_64
            Qt5.6.1/5.6.1/gcc_64
            Qt5.6.1-1/5.6.1-1/gcc_64
            Qt5.6.2/5.6.2/gcc_64
            Qt5.7.0/5.7.0/gcc_64
            Qt5.7.1/5.7.1/gcc_64
            Qt5.8.0/5.8.0/gcc_64


            qt/5.3/gcc
            qt/5.3/gcc_64
            5.3/gcc
            5.3/gcc_64
            5.4/gcc_64
            5.4/gcc
            5.5/gcc_64
            5.5/gcc

            qt5
    )
endif()

# ==================================================
# brute force
# ==================================================
FIND_PATH(CMAKE_PREFIX_PATH ${QT5_CORE_LIB}
    PATH_SUFFIXES ${QT5_PATH_SUFFIXES}
    PATHS ${QT5_PATHS}
)
message(STATUS "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")


# ============================================================================
# determine Qt5 modules' library and include directories
# ============================================================================
if(NOT CMAKE_PREFIX_PATH)
	SET(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE PATH "Qt5 install dir")
else()

    SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH}/../ CACHE PATH "Qt5 install dir")

	SET(QT5_COMP_LIST
	    "Qt5Core"
	    "Qt5Widgets"
	    "Qt5Xml"
	    "Qt5Gui"
	    "Qt5Concurrent"
        "Qt5Sql"
	    #"Qt5Declarative"
	    #"Qt5Qml"
	    #"Qt5Quick"
	)
	
	foreach(QT5COMP ${QT5_COMP_LIST})
	    find_package(${QT5COMP})
	    if(${QT5COMP}_FOUND)
			if(WIN32)
				LIST(APPEND QT5_LIB_LIST "${QT5COMP}.lib")
			else(WIN32)
				LIST(APPEND QT5_LIB_LIST "${QT5COMP}_LIBRARIES")
		    endif(WIN32)
	        LIST(APPEND QT5_INCLUDE_DIRS ${${QT5COMP}_INCLUDE_DIRS})

            #			string(COMPARE EQUAL "${QT5COMP}" Qt5Sql _cmp)
            #            if (_cmp)
            #                list(LENGTH ${QT5COMP}_PLUGINS numplugs)
            #                if (${numplugs})
            #                    list(GET Qt5Sql_PLUGINS 0 plugin)
            #                    get_target_property(_loc ${plugin} LOCATION)
            #                    get_filename_component(_loc_path ${_loc} DIRECTORY)
            #                    message("Plugin ${plugin} is at location ${_loc_path}")
            #                    set(QT5SQL_PLUGINS_DIR ${_loc_path} CACHE FILEPATH
            #                        "Path to Qt5's qsqlite.<so | dll | lib> plugin library" FORCE)
            #                endif()
            #			endif()
	    endif(${QT5COMP}_FOUND)
	endforeach(QT5COMP)
	list(REMOVE_DUPLICATES QT5_INCLUDE_DIRS)
	
	message(STATUS "Qt5 libraries: ${QT5_LIB_LIST}")
	message(STATUS "Qt5 INCLUDE_DIRS: ${QT5_INCLUDE_DIRS}")
	
	# extract the link directories
	get_target_property(FULLLIBNAME ${Qt5Core_LIBRARIES} LOCATION)
	get_filename_component(LIBPATH ${FULLLIBNAME} PATH)
	SET(QT5_LINK_DIRS ${LIBPATH})
	foreach(LOOPLIB ${QT5_LIB_LIST})
		get_target_property(FULLLIBNAME ${LOOPLIB} LOCATION)
		get_filename_component(LIBPATH ${FULLLIBNAME} PATH)
	    list(APPEND QT5_LINK_DIRS ${LIBPATH})
	endforeach(LOOPLIB)
	list(REMOVE_DUPLICATES QT5_LINK_DIRS)
	
	# just from experience
	IF(WIN32)
		set(WINLIBDIR ${CMAKE_PREFIX_PATH}/lib)
		list(APPEND QT5_LINK_DIRS ${WINLIBDIR})
	ENDIF(WIN32)
	
	message(STATUS "Qt5 link directories: ${QT5_LINK_DIRS}")
endif()


# ==========================================================
# Couldn't find the plugins dir: try harder now !
# ===========================================================
#if(NOT QT5SQL_PLUGINS_DIR)

#    #message(STATUS "... looking for ${QT5SQLite_PLUGIN_LIBRARY}")
#    #message(STATUS "... in PATHS: ${CMAKE_PREFIX_PATH}")
#    #message(STATUS "... and sub PATHS: ${QT5_PATH_SUFFIXES}")

#    FIND_PATH(QT5SQL_PLUGINS_DIR plugins/sqldrivers/${QT5SQLite_PLUGIN_LIBRARY}
#        PATH_SUFFIXES ${QT5_PATH_SUFFIXES}
#        PATHS ${CMAKE_PREFIX_PATH}
#    )

#    if (NOT QT5SQL_PLUGINS_DIR)
#        message(FATAL_ERROR "Please specify the location to Qt5's qsqlite plugin library (*.dll, *.so, *.lib)")
#    endif()
#endif()



# ==========================================================
# find the private header dir for Qt5Core and Qt5Sql
# ==========================================================
foreach(INCLDIR ${QT5_INCLUDE_DIRS})
    FIND_PATH(QT5CORE_PRIVATE_DIR private/qitemselectionmodel_p.h
        PATH_SUFFIXES
            5.0.0/QtCore
            5.0.1/QtCore
            5.0.2/QtCore
            5.1.0/QtCore
            5.1.1/QtCore
            5.2.0/QtCore
            5.2.1/QtCore
            5.3.0/QtCore
            5.3.1/QtCore
            5.3/QtCore
            5.4.0/QtCore
            5.4.1/QtCore
            5.4.2/QtCore
            5.5.0/QtCore
            5.5.1/QtCore
            5.6.1/QtCore
            5.6.1-1/QtCore
            5.6.2/QtCore
            5.7.0/QtCore
            5.7.1/QtCore
            5.8.0/QtCore
        PATHS 
            ${INCLDIR}
            c:/qt/5.5.0/qtbase/include/QtCore
            c:/Qt/5.5.0/qtbase/include/QtCore
            c:/qt/5.5.1/qtbase/include/QtCore
            c:/qt/5.6.1/qtbase/include/QtCore
            c:/qt/5.6.1-1/qtbase/include/QtCore
            c:/qt/5.6.2/qtbase/include/QtCore
            c:/qt/5.7.0/qtbase/include/QtCore
            c:/qt/5.7.1/qtbase/include/QtCore
            c:/qt/5.8.0/qtbase/include/QtCore
        NO_DEFAULT_PATH
    )
endforeach()

if(NOT QT5CORE_PRIVATE_DIR)
    message(STATUS "couldn't find QT5CORE_PRIVATE_DIR!")
else()
    message(STATUS "QT5CORE_PRIVATE_DIR=${QT5CORE_PRIVATE_DIR}")
endif()

# find the private header dir for QtSql
foreach(INCLSQL ${QT5_INCLUDE_DIRS})
    FIND_PATH(QT5SQL_INCLUDE_DIR QtSql/private/qsql_sqlite_p.h
        PATH_SUFFIXES
            5.0.0
            5.0.1
            5.0.2
            5.1.0
            5.1.1
            5.2.0
            5.2.1
            5.3.0
            5.3.1
              5.3
            5.4.0
            5.4.1
            5.4.2
            5.5.0
            5.5.1
            5.6.1
            5.6.1-1
            5.6.2
            5.7.0
            5.7.1
            5.8.0
        PATHS
            ${INCLSQL}
            c:/qt/5.5.0/qtbase/include/QtSql
            c:/Qt/5.5.0/qtbase/include/QtSql
            c:/Qt/5.5.1/qtbase/include/QtSql
            c:/qt/5.5.1/qtbase/include/QtSql
            c:/qt/5.6.1/qtbase/include/QtSql
            c:/qt/5.6.1-1/qtbase/include/QtSql
            c:/qt/5.6.2/qtbase/include/QtSql
            c:/qt/5.7.0/qtbase/include/QtSql
            c:/qt/5.7.1/qtbase/include/QtSql
            c:/qt/5.8.0/qtbase/include/QtSql
        NO_DEFAULT_PATH
    )
endforeach()

if(NOT QT5SQL_INCLUDE_DIR)
    set(QT5SQL_INCLUDE_DIR ${QT5SQL_INLCUDE_DIR}
        CACHE FILEPATH "Path to QtSql include directory" FORCE)
    message(STATUS "Couldn't find QtSql inlcude directory!")
endif()

# find the private header dir QtWidgets
#foreach(INCLWIDGET ${QT5_INCLUDE_DIRS})
#    FIND_PATH(QT5WIDGETS_INCLUDE_DIR QtWidgets/private/qcompleter_p.h
#        PATH_SUFFIXES
#            5.0.0
#            5.0.1
#            5.0.2
#            5.1.0
#            5.1.1
#            5.2.0
#            5.2.1
#            5.3.0
#            5.3.1
#              5.3
#            5.4.0
#            5.4.1
#            5.4.2
#            5.5.0
#            5.5.1
#            5.6.1
#        PATHS
#            ${INCLWIDGET}
#            c:/qt/5.5.0/qtbase/include/QtWidgets
#            c:/Qt/5.5.0/qtbase/include/QtWidgets
#            c:/Qt/5.5.1/qtbase/include/QtWidgets
#            c:/qt/5.5.1/qtbase/include/QtWidgets
#            c:/qt/5.6.1/qtbase/include/QtWidgets
#        NO_DEFAULT_PATH
#    )
#endforeach()

#if(NOT QT5WIDGETS_INCLUDE_DIR)
#    set(QT5WIDGETS_INCLUDE_DIR ${QT5WIDGETS_INLCUDE_DIR}
#        CACHE FILEPATH "Path to QtWidgets include directory" FORCE)
#    message(STATUS "Couldn't find QtWidgets inlcude directory!")
#endif()

# have a look around whether we can find a source 
# directory hosting the QSQLiteDriver code
# FIND_PATH(QT5SQLite_SRC_DIR qsql_sqlite.cpp
    # PATH_SUFFIXES
        # Src/qtbase/src/sql/drivers/sqlite
		# src/sql/drivers/sqlite
    # PATHS
        # /opt/Qt/5.4
        # /opt/qt/5.4
		# /opt/qt/5.5
        # c:/Qt/5.4/qtbase
        # c:/qt/5.4/qtbase
		# c:/qt/5.5.0/qtbase
# )

# if we've got sources for the QSQLiteDriver installed
# we copy the appropriate files into the lumass source tree
# for compilation
# IF(QT5SQLite_SRC_DIR)
    # file(COPY ${QT5SQLite_SRC_DIR}/qsql_sqlite_p.h
			  # ${QT5SQLite_SRC_DIR}/qsql_sqlite.cpp
         # DESTINATION ${utils_SOURCE_DIR}/Qt/drivers/sqlite)
		 
	# set(NMQSQLite_SRC_DIR ${utils_SOURCE_DIR}/Qt/drivers/sqlite
		# CACHE FILEPATH "Path to QSQLiteDriver source file" FORCE)	
# ENDIF()

# if we couldn't find the QSQLiteDriver's source files locally,
# we try to download them from github.com/qtproject
# file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/utils/Qt/drivers/sqlite)
# if(NOT NMQSQLite_SRC_DIR)
#     file(DOWNLOAD
#          https://github.com/qtproject/qtbase/raw/dev/src/sql/drivers/sqlite/qsql_sqlite.cpp
#          ${lumass_SOURCE_DIR}/utils/Qt/drivers/sqlite/nmqsql_sqlite.cpp
#          STATUS down_cpp SHOW_PROGRESS)
#     file(DOWNLOAD
#          https://github.com/qtproject/qtbase/raw/dev/src/sql/drivers/sqlite/qsql_sqlite_p.h
#          ${lumass_SOURCE_DIR}/utils/Qt/drivers/sqlite/nmqsql_sqlite_p.h
#          STATUS down_h SHOW_PROGRESS)
#     file(DOWNLOAD
#          https://github.com/qtproject/qtbase/raw/dev/src/sql/kernel/qsqlcachedresult.cpp
#          ${lumass_SOURCE_DIR}/utils/Qt/drivers/sqlite/nmqsqlcachedresult.cpp
#          STATUS cache_down_cpp SHOW_PROGRESS)
#     file(DOWNLOAD
#          https://github.com/qtproject/qtbase/raw/dev/src/sql/kernel/qsqlcachedresult_p.h
#          ${lumass_SOURCE_DIR}/utils/Qt/drivers/sqlite/nmqsqlcachedresult_p.h
#          STATUS cache_down_h SHOW_PROGRESS)

	
#     list(GET down_cpp 0 down_cpp_err)
#     list(GET down_h 0 down_h_err)
#     list(GET cache_down_cpp 0 cache_down_cpp_err)
#     list(GET cache_down_h 0 cache_down_h_err)

#     if(NOT down_cpp_err OR NOT down_h_err
#         OR cache_down_cpp_err OR cache_down_h_err
#       )
#         set(NMQSQLite_SRC_DIR ${lumass_SOURCE_DIR}/utils/Qt/drivers/sqlite
#             CACHE FILEPATH "Path to QSQLiteDriver source file" FORCE)
#     endif()
# endif()

# ==========================================================
# finally, we determine the qt5 version
# ==========================================================

# let's find the path to Qt5ConfigVersion
FIND_PATH(QT5_CONFIG_PATH Qt5ConfigVersion.cmake
        PATH_SUFFIXES
           cmake/Qt5
           lib/cmake/Qt5

        PATHS
            ${CMAKE_PREFIX_PATH}
            ${CMAKE_PREFIX_PATH}/..
        NO_DEFAULT_PATH
)
message(STATUS "QT5_CONFIG_PATH: ${QT5_CONFIG_PATH}")

if(QT5_CONFIG_PATH-NOTFOUND)
    message(STATUS "Couldn't find Qt5ConfigVersion.cmake!")
    set(QT5_VERSION_STRING "5.?.?")
else()
    include(${QT5_CONFIG_PATH}/Qt5ConfigVersion.cmake)
    set(QT5_VERSION_STRING "${PACKAGE_VERSION}")
    message(STATUS "Qt5 version: ${QT5_VERSION_STRING}")
endif()

