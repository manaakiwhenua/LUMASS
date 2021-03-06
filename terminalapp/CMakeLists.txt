PROJECT(lumassengine)

find_package(VTK COMPONENTS
vtkCommonCore
vtkCommonDataModel
vtkIOCore
vtkIOSQL
vtkIOLegacy
vtkIOXML
)

include(${VTK_USE_FILE})

INCLUDE_DIRECTORIES(
    ${QT5_INCLUDE_DIRS}
	${opt_SOURCE_DIR}
	${gui_SOURCE_DIR}
	${shared_SOURCE_DIR}
	${lumass_BINARY_DIR}
	${lumass_SOURCE_DIR}
 	${GDALRATImageIO_SOURCE_DIR}
	${GDALRATImageIO_BINARY_DIR}
        ${otbsuppl_core_SOURCE_DIR}
	${filters_SOURCE_DIR}
	${filters_BINARY_DIR}
	${shared_SOURCE_DIR}
        ${shared_BINARY_DIR}
	${rimageio_SOURCE_DIR}
        ${mfw_core_SOURCE_DIR}
        ${mfw_core_BINARY_DIR}
	${mfw_wrapper_SOURCE_DIR}
        ${mfw_wrapper_BINARY_DIR}
        ${LPSOLVE_INCLUDE_DIR}
	${OTB_INCLUDE_DIRS}
	${RASDAMAN_INCLUDE_DIR}                                   
	${RASDAMAN_INCLUDE_DIR}/raslib	
	${RASDAMAN_SOURCE_DIR}/applications/rasgeo
	${PostgreSQL_INCLUDE_DIRS}
)     

LINK_DIRECTORIES(
    ${QT5_LINK_DIRS}
    ${OTB_LIBRARY_DIRS}
    ${LIBRARY_OUTPUT_PATH}
    ${muparser_BINARY_DIR}
    ${RASDAMAN_LIBRARIES_DIR}
    ${PostgreSQL_LIBRARY_DIRS}
    ${OSGEO4W_ROOT}/lib
#    ${SPATIALITE_LIB_DIR}
    ${SPATIALITE_LIBLIB_DIR}
)

# Qt5 requirement
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -fPIC")

QT5_WRAP_CPP(ENGINE_MOC_CPP
	${opt_SOURCE_DIR}/NMMosra.h
	${opt_SOURCE_DIR}/MOSORunnable.h
        ${shared_SOURCE_DIR}/NMLogger.h
        #${shared_SOURCE_DIR}/NMObject.h
        ${lumassengine_SOURCE_DIR}/lumassengine.h
)

file(GLOB ENGINE_SRC 
    ${opt_SOURCE_DIR}/*.cpp 
    ${lumassengine_SOURCE_DIR}/*.cxx 

    # and ...
    ${shared_SOURCE_DIR}/*.cpp
)

set(ENGINE_LINKLIBS Qt5Core Qt5Sql Qt5Xml
        NMModFrameCore ${MFW_WRAPPER_NAMES}
        ${VTK_LIBRARIES} ${PostgreSQL_LIBRARIES}
        ${LPSOLVE_LIBRARY}
        #spatialite
)

# need to add the import library on windows
#if(WIN32)
#	LIST(APPEND ENGINE_LINKLIBS spatialite_i)
#endif()
	
set(ENGINE_DEPLIBS
    NMModFrameCore ${MFW_WRAPPER_NAMES}
)

IF(RASSUPPORT)
    add_definitions(-DBUILD_RASSUPPORT)
    LIST(APPEND ENGINE_LINKLIBS raslib RasImageIO)
ENDIF()


# VTK6 requirement
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS ${VTK_DEFINITIONS})

ADD_EXECUTABLE(lumassengine ${ENGINE_SRC} ${ENGINE_MOC_CPP})
#set_property(TARGET lumassengine PROPERTY POSITION_INDEPENDENT_CODE 1)
TARGET_LINK_LIBRARIES(lumassengine ${ENGINE_LINKLIBS})

install(TARGETS lumassengine DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
