PROJECT(mfw_core)

if (${VTK_VERSION_STRING} VERSION_LESS "8.90")
    SET(MFW_CORE_VTK_LIBRARIES
        vtkCommonDataModel
        vtkCommonCore
        vtkIOCore
    )
        include(${VTK_USE_FILE})
else()
   SET(MFW_CORE_VTK_LIBRARIES
       VTK::CommonDataModel
       VTK::CommonCore
       VTK::IOCore
   )
endif()

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
    ${NETCDFIO_SOURCE_DIR}
    ${NETCDFIO_BINARY_DIR}
    ${NETCDF_INCLUDE_DIR}
    ${NCXX4_INCLUDE_DIR}
    #"/opt/netcdf-c-4.8.1/install/include"
#    ${rimageio_SOURCE_DIR}
    ${filters_SOURCE_DIR}
    ${filters_BINARY_DIR}
    ${gui_SOURCE_DIR}
    ${lumass_SOURCE_DIR}/utils/ITK
    ${lumass_SOURCE_DIR}
    ${lumass_SOURCE_DIR}/modellingframework/wrapper
    ${OTBSupplCore_SOURCE_DIR}
    ${OTBSupplCore_BINARY_DIR}
    ${OTB_INCLUDE_DIRS}
    ${pybind11_INCLUDE_DIRS}
    ${shared_BINARY_DIR}
    ${MPI_CXX_INCLUDE_DIRS}
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
    ${NETCDF_LIB_DIR}
    ${NCXX4_LIB_DIR}
)

file(GLOB MFW_CORE_CPP
   ${mfw_core_SOURCE_DIR}/*.cpp
   ${mfw_wrapper_SOURCE_DIR}/NMImageReader.cxx
   ${mfw_wrapper_SOURCE_DIR}/NMTableReader.cpp
   ${mfw_wrapper_SOURCE_DIR}/NMItkDataObjectWrapper.cpp
   ${shared_SOURCE_DIR}/NMLogger.cpp
   ${shared_SOURCE_DIR}/NMObject.cpp
)

LIST(REMOVE_ITEM MFW_CORE_CPP ${mfw_core_SOURCE_DIR}/NMConditionalIterComponent.cpp)

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
    Qt5::Xml
    Qt5::Core
    Qt5::Concurrent
    NMNetCDFIO
    NMOTBSupplCore
    #${OTB_LIBRARIES}
    #${OTB_LINK_LIBS}
    NMOTBGDALRATImageIO
    NMOTBSupplFilters
    #RasImageIO
    #ITKCommon
    #spatialite
)

if(PYTHONSUPPORT)
    list(APPEND MFW_CORE_LINK_LIBS
            ${Python3_LIBRARIES}
                        pybind11::pybind11
            pybind11::embed
        )

endif()

set(MFW_CORE_DEP_LIBS
    NMNetCDFIO
    NMOTBGDALRATImageIO
    NMOTBSupplFilters
    #RasImageIO
)

SET(CMAKE_AUTOMOC YES)

#IF(RASSUPPORT)
#    add_definitions(-DBUILD_RASSUPPORT)
#    LIST(APPEND MFW_LINK_LIBS RasImageIO)
#    LIST(APPEND MFW_DEP_LIBS RasImageIO)
#ENDIF(RASSUPPORT)

if (PYTHONSUPPORT)
    add_definitions(-DLUMASS_PYTHON)
endif()

add_definitions(-DNMModFrameCore_EXPORTS -DNM_PROC_LOG)

include(GenerateExportHeader)
ADD_LIBRARY(NMModFrameCore SHARED ${MFW_CORE_CPP})
generate_export_header(NMModFrameCore)

target_link_libraries(NMModFrameCore ${MFW_CORE_LINK_LIBS})
ADD_DEPENDENCIES(NMModFrameCore ${MFW_CORE_DEP_LIBS})

install(FILES ${MFW_CORE_H} DESTINATION include)
if(WIN32)
        install(TARGETS NMModFrameCore
                LIBRARY DESTINATION lib
                RUNTIME DESTINATION lib
        )
else()
        install(TARGETS NMModFrameCore LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif()
