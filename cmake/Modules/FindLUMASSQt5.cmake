# *******************************************************
# FindLUMASSQt5.cmake
# based on cmake 2.8.9
# finds main installation path of custom installed Qt5 (Qt-Project webiste download)
# ****************************************************

# CUSTQT5_FOUND

FIND_PATH(CMAKE_PREFIX_PATH lib/libQt5Core.la
	PATH_SUFFIXES
                Qt5.2.1/5.2.1/gcc_64
                Qt5.2.1/5.2.1/gcc

                Qt5.2.0/5.2.0/gcc_64
                Qt5.2.0/5.2.0/gcc

                Qt5.1.0/5.1.0/gcc_64
                Qt5.1.0/5.1.0/gcc

                Qt5.1.1/5.1.1/gcc_64
                Qt5.1.1/5.1.1/gcc

	PATHS
                /opt
                /usr
                /usr/local
)

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
	    "Qt5Declarative"
	    "Qt5Qml"
	    "Qt5Quick"
	)
	
	foreach(QT5COMP ${QT5_COMP_LIST})
	    find_package(${QT5COMP})
	    if(${QT5COMP}_FOUND)
	        LIST(APPEND QT5_LIB_LIST "${QT5COMP}_LIBRARIES")
	        LIST(APPEND QT5_INCLUDE_DIRS ${${QT5COMP}_INCLUDE_DIRS})
	    endif(${QT5COMP}_FOUND)
	endforeach(QT5COMP)
	list(REMOVE_DUPLICATES QT5_INCLUDE_DIRS)
	
	message(STATUS "qt5 lib list: ${QT5_LIB_LIST}")
	message(STATUS "qt5 include dirs: ${QT5_INCLUDE_DIRS}")
	
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
	
	message(STATUS "qt5 link dirs: ${QT5_LINK_DIRS}")
endif()
