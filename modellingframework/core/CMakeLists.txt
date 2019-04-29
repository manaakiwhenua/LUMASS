PROJECT(mfw_core)

find_package(VTK COMPONENTS
        vtkCommonDataModel
        vtkCommonCore
        vtkIOCore
)

include(${VTK_USE_FILE})

INCLUDE_DIRECTORIES(
    ${SPATIALITE_INCLUDE_DIR}
    ${QT5_INCLUDE_DIRS}
#    ${PostgreSQL_INCLUDE_DIRS}
#    ${RASDAMAN_SOURCE_DIR}/applications/rasgeo
#    ${RASDAMAN_INCLUDE_DIR}
#    ${RASDAMAN_INCLUDE_DIR}/raslib
    ${VTK_INCLUDE_DIRS}

    ${mfw_core_BINARY_DIR}
    ${mfw_core_SOURCE_DIR}
    ${mfw_wrapper_SOURCE_DIR}
    ${mfw_wrapper_BINARY_DIR}
#    ${muParserX_SOURCE_DIR}
    ${shared_SOURCE_DIR}
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
#    ${rimageio_SOURCE_DIR}
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
    ${ITK_LIBRARY_DIRS}
    ${ITK_LIBRARY_DIRS}/../
    ${QT5_LINK_DIRS}
#    ${RASDAMAN_LIBRARIES_DIR}
    ${LIBRARY_OUTPUT_PATH}
    ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}
    ${OSGEO4W_ROOT}/lib
    ${SPATIALITE_LIB_DIR}
    ${SPATIALITE_LIBLIB_DIR}
)

file(GLOB MFW_CORE_CPP
   ${mfw_core_SOURCE_DIR}/*.cpp
   ${mfw_wrapper_SOURCE_DIR}/NMImageReader.cxx
   ${mfw_wrapper_SOURCE_DIR}/NMTableReader.cpp
   ${mfw_wrapper_SOURCE_DIR}/NMItkDataObjectWrapper.cpp
   ${shared_SOURCE_DIR}/NMLogger.cpp
   ${shared_SOURCE_DIR}/NMObject.cpp
)

file(GLOB MFW_CORE_H
   ${mfw_core_SOURCE_DIR}/*.h
   ${mfw_core_BINARY_DIR}/*.h
   ${mfw_wrapper_SOURCE_DIR}/NMImageReader.h
   ${mfw_wrapper_SOURCE_DIR}/NMTableReader.h
   ${mfw_wrapper_SOURCE_DIR}/NMItkDataObjectWrapper.h

   ${shared_SOURCE_DIR}/NMLogger.h
   ${shared_SOURCE_DIR}/NMObject.h
)


#if (NOT RASSUPPORT)
#  list(REMOVE_ITEM MFW_H
#  	"${mfw_wrapper_SOURCE_DIR}/NMRasdamanConnectorWrapper.h"
#  )
  
#  list(REMOVE_ITEM MFW_CPP
#        "${mfw_wrapper_SOURCE_DIR}/NMRasdamanConnectorWrapper.cpp"
#  )
#endif()
    

SET(MFW_CORE_MOC_H ${MFW_CORE_H})
LIST(REMOVE_ITEM MFW_CORE_MOC_H ${mfw_core_SOURCE_DIR}/NMMfwException.h)
LIST(APPEND MFW_CORE_MOC_H ${shared_SOURCE_DIR}/NMLogger.h)

set(MFW_CORE_LINK_LIBS
                OTBGDALRATImageIO
                OTBSupplFilters
		#RasImageIO
		Qt5Xml
		Qt5Core
		Qt5Concurrent
                ITKCommon
                #spatialite
)

set(MFW_CORE_DEP_LIBS
    OTBGDALRATImageIO
    OTBSupplFilters
    #RasImageIO
)

QT5_WRAP_CPP(MFW_CORE_MOC_CPP ${MFW_CORE_MOC_H} )

#IF(RASSUPPORT)
#    add_definitions(-DBUILD_RASSUPPORT)
#    LIST(APPEND MFW_LINK_LIBS RasImageIO)
#    LIST(APPEND MFW_DEP_LIBS RasImageIO)
#ENDIF(RASSUPPORT)

add_definitions(-DNMModFrameCore_EXPORTS -DNM_PROC_LOG)
if (LUMASS_OTB_VERSION VERSION_GREATER "5.8.0")
    add_definitions(-DOTB_VERSION_SIX)
endif()

include(GenerateExportHeader)
ADD_LIBRARY(NMModFrameCore SHARED ${MFW_CORE_CPP} ${MFW_CORE_MOC_CPP})
generate_export_header(NMModFrameCore)

TARGET_LINK_LIBRARIES(NMModFrameCore ${MFW_CORE_LINK_LIBS})
ADD_DEPENDENCIES(NMModFrameCore ${MFW_CORE_DEP_LIBS})

install(FILES ${MFW_CORE_H} DESTINATION include)
if(WIN32)
        install(TARGETS NMModFrameCore
		LIBRARY DESTINATION lib
		RUNTIME DESTINATION lib
	)
else()
        install(TARGETS NMModFrameCore LIBRARY DESTINATION lib)
endif()
