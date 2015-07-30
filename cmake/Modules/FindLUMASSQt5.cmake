# *******************************************************
# FindLUMASSQt5.cmake
# based on cmake 2.8.9
# finds main installation path of custom installed Qt5 (Qt-Project webiste download)
# ****************************************************

# CUSTQT5_FOUND

if(WIN32)
	set(QT5_CORE_LIB "lib/Qt5Core.lib")
	set(QT5_PATHS
		C:/qt
		C:/
		C:/Qt
	)
else()
        set(QT5_CORE_LIB "libQt5Core.so")
	set(QT5_PATHS
		/opt
		/opt/qt
		/usr
		/usr/qt
		/usr/local
                /usr/lib
                /usr/lib/x86_64-linux-gnu
	)
endif()

# brute force
FIND_PATH(CMAKE_PREFIX_PATH ${QT5_CORE_LIB}
        PATH_SUFFIXES
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
               
                qt/5.3/gcc
                qt/5.3/gcc_64
                5.3/gcc
                5.3/gcc_64
				
				Qt5.3.1/qtbase
				Qt5.3.1/5.3/qtbase
				5.3.1/qtbase
				5.3.1/5.3/qtbase
	PATHS		
				${QT5_PATHS}
)
message(STATUS "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")

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

# find the private header dir for QtCore and QtGui
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
        PATHS ${INCLDIR}
        NO_DEFAULT_PATH
    )
endforeach()

if(NOT QT5CORE_PRIVATE_DIR)
    message(STATUS "couldn't find QT5CORE_PRIVATE_DIR!")
else()
    message(STATUS "QT5CORE_PRIVATE_DIR=${QT5CORE_PRIVATE_DIR}")
endif()

# finally, we determine the qt5 version
include(${CMAKE_PREFIX_PATH}/lib/cmake/Qt5/Qt5ConfigVersion.cmake)
set(QT5_VERSION_STRING "${PACKAGE_VERSION}")
