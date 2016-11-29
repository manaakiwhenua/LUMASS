PROJECT(mfw)

ADD_SUBDIRECTORY(wrapper ${mfw_BINARY_DIR}/wrapper)

INCLUDE_DIRECTORIES(
    ${SPATIALITE_INCLUDE_DIR}
	${QT5_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
    ${RASDAMAN_SOURCE_DIR}/applications/rasgeo
    ${RASDAMAN_INCLUDE_DIR}
    ${RASDAMAN_INCLUDE_DIR}/raslib
    ${VTK_INCLUDE_DIRS}
    ${mfw_BINARY_DIR}
    ${mfw_SOURCE_DIR}
    ${mfw_wrapper_SOURCE_DIR}
    ${muParserX_SOURCE_DIR}
    ${shared_SOURCE_DIR}
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${rimageio_SOURCE_DIR}
    ${filters_SOURCE_DIR}
    ${filters_BINARY_DIR}
    ${gui_SOURCE_DIR}
    ${lumass_SOURCE_DIR}/utils/ITK
    ${lumass_SOURCE_DIR}
    ${otbsuppl_core_SOURCE_DIR}
    ${OTB_INCLUDE_DIRS}
)

LINK_DIRECTORIES(
    ${OTB_DIR}/lib
    ${OTB_DIR}/bin
    ${QT5_LINK_DIRS}
    ${RASDAMAN_LIBRARIES_DIR}
    ${LIBRARY_OUTPUT_PATH}
    ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}
    ${OSGEO4W_ROOT}/lib
    ${SPATIALITE_LIB_DIR}
    ${SPATIALITE_LIBLIB_DIR}
)

file(GLOB MFW_CPP
   ${mfw_SOURCE_DIR}/*.cpp
   ${mfw_wrapper_SOURCE_DIR}/*.cpp
   ${mfw_wrapper_SOURCE_DIR}/*.cxx
   ${lumass_SOURCE_DIR}/util/ITK/itkVTKImageExportBase.cxx
   ${shared_SOURCE_DIR}/NMLogger.cpp
)

file(GLOB MFW_H
   ${mfw_SOURCE_DIR}/*.h
   ${mfw_BINARY_DIR}/*.h
   ${mfw_wrapper_SOURCE_DIR}/*.h
   ${shared_SOURCE_DIR}/NMLogger.h
)

    #LIST(REMOVE_ITEM MFW_CPP ${mfw_wrapper_SOURCE_DIR}/WrapperTemplate.cpp)
    #LIST(REMOVE_ITEM MFW_H ${mfw_wrapper_SOURCE_DIR}/WrapperTemplate.h)
   
if (NOT RASSUPPORT)
  list(REMOVE_ITEM MFW_H 
  	"${mfw_wrapper_SOURCE_DIR}/NMRasdamanConnectorWrapper.h"
  )
  
  list(REMOVE_ITEM MFW_CPP
        "${mfw_wrapper_SOURCE_DIR}/NMRasdamanConnectorWrapper.cpp"
  )
endif()   
    

SET(MFW_MOC_H ${MFW_H})
LIST(REMOVE_ITEM MFW_MOC_H ${mfw_SOURCE_DIR}/NMMfwException.h)
LIST(APPEND MFW_MOC_H ${shared_SOURCE_DIR}/NMLogger.h)


#if (OTB_VERSION_STRING VERSION_LESS "5.0.0")
#	set(MFW_LINK_LIBS
#		OTBGDALRATImageIO
#		OTBSupplFilters
#		#RasImageIO
#		Qt5Xml
#		Qt5Core
#		Qt5Concurrent
#		spatialite
#	)
#else()
	set(MFW_LINK_LIBS
		OTBGDALRATImageIO
                #OTBMathParser
		OTBSupplFilters
		#RasImageIO
		Qt5Xml
		Qt5Core
		Qt5Concurrent
                #spatialite
	)
#endif()

# need to add the import library on windows
#if(WIN32)
#	LIST(APPEND MFW_LINK_LIBS spatialite_i)
#endif()

set(MFW_DEP_LIBS
    OTBGDALRATImageIO
    OTBSupplFilters
    #RasImageIO
)

QT5_WRAP_CPP(MFW_MOC_CPP ${MFW_MOC_H})

IF(RASSUPPORT)
    add_definitions(-DBUILD_RASSUPPORT)
    LIST(APPEND MFW_LINK_LIBS RasImageIO)
    LIST(APPEND MFW_DEP_LIBS RasImageIO)
ENDIF()

add_definitions(-DNMModFrame_EXPORTS -DNM_PROC_LOG)
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -fPIC")
#IF(NOT WIN32)
#    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-implicit-templates")
#ENDIF(NOT WIN32)

include(GenerateExportHeader)
	# not quite sure whether we need the below
	#set (EXTRA_FLAGS "-fvisibility=hidden")
ADD_LIBRARY(NMModFrame SHARED ${MFW_CPP} ${MFW_MOC_CPP})

generate_export_header(NMModFrame)

#QT5_USE_MODULES(NMModFrame Core Xml Concurrent)
TARGET_LINK_LIBRARIES(NMModFrame ${MFW_LINK_LIBS})
ADD_DEPENDENCIES(NMModFrame ${MFW_DEP_LIBS})

install(FILES ${MFW_H} DESTINATION include)
if(WIN32)
	install(TARGETS NMModFrame 
		LIBRARY DESTINATION lib
		RUNTIME DESTINATION lib
	)
else()
	install(TARGETS NMModFrame LIBRARY DESTINATION lib)
endif()
