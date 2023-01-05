PROJECT(filters)

INCLUDE_DIRECTORIES(
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${NCXX4_INCLUDE_DIRS}
    ${NETCDF_INCLUDE_DIRS}
    ${filters_SOURCE_DIR}
    ${filters_BINARY_DIR}
    ${shared_SOURCE_DIR}
    ${QT5_INCLUDE_DIRS}
    ${OTB_DIR}/Code/Common
    ${muparser_SOURCE_DIR}
    ${muParserX_SOURCE_DIR}
    ${otbsuppl_core_SOURCE_DIR}
    ${utils_SOURCE_DIR}
    ${OTB_DIR}/Utilities/ITK/Utilities/vxl/core/vnl
    ${lumass_SOURCE_DIR}/utils/ITK
    ${lumass_SOURCE_DIR}
    #${TCADB_INCLUDE_DIR}
    ${PYBIND11_INCLUDE_DIR}
    ${MPI_CXX_INCLUDE_DIRS}
)

LINK_DIRECTORIES(
    ${OTB_BINARY_DIR}
    ${OTB_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    ${OTB_DIR}/bin
    ${OTB_DIR}/bin/${CMAKE_BUILD_TYPE}
    ${OTB_DIR}/lib
    ${SPATIALITE_LIB_DIR}
    ${SPATIALITE_LIBLIB_DIR}

    #${TCADB_LIB_DIR}
)

file(GLOB OTBSUPPL_FILTERS
    ${filters_SOURCE_DIR}/*.txx
    ${filters_SOURCE_DIR}/*.cxx
    ${muparser_SOURCE_DIR}/*.cpp
    ${muParserX_SOURCE_DIR}/*.cpp
    ${otbsuppl_core_SOURCE_DIR}/*.cxx
    ${otbsuppl_core_SOURCE_DIR}/*.hxx
)

file(GLOB OTBSUPPL_HEADER
    ${filters_SOURCE_DIR}/*.h
    ${filters_BINARY_DIR}/*.h
)

include(GenerateExportHeader)

add_definitions(-DNM_PROC_LOG)

if(WIN32)
        add_definitions(-DOTBGDALRATIMAGEIO_STATIC_DEFINE)
        ADD_LIBRARY(OTBSupplFilters STATIC ${OTBSUPPL_FILTERS})
else(WIN32)
        ADD_LIBRARY(OTBSupplFilters SHARED ${OTBSUPPL_FILTERS})
endif(WIN32)

set_target_properties(OTBSupplFilters PROPERTIES
                        LINKER_LANGUAGE CXX)

generate_export_header(OTBSupplFilters)

if(LUMASS_OTB_VERSION VERSION_LESS "5.0.0")
    if(WIN32)
        target_link_libraries(OTBSupplFilters OTBCommon OTBBasicFilters OTBIO
        OTBGDALRATImageIO
        #tokyocabinet
        )
    else()
        target_link_libraries(OTBSupplFilters OTBCommon OTBBasicFilters OTBIO
        OTBGDALRATImageIO
        ${MPI_CXX_LIBRARIES}
        )
    endif()
else()
    if(WIN32)
        target_link_libraries(OTBSupplFilters #OTBCommon OTBIOGDAL
        #OTBExtendedFilename OTBCurlAdapters OTBImageIO OTBImageBase
        #OTBMathParser
        #OTBStreaming
        OTBGDALRATImageIO
        Qt5Core Qt5Qml
        #tokyocabinet
        )
    else()
        target_link_libraries(OTBSupplFilters #OTBCommon OTBIOGDAL
        OTBGDALRATImageIO
        Qt5Core Qt5Qml
        ${MPI_CXX_LIBRARIES}
        )
    endif()
endif()

add_dependencies(OTBSupplFilters OTBGDALRATImageIO)

#set_property(TARGET OTBSupplFilters PROPERTY POSITION_INDEPENDENT_CODE 1)



#install(FILES ${OTBSUPPL_HEADER} DESTINATION include)
if(WIN32)
        install(TARGETS OTBSupplFilters
                #LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib
                RUNTIME DESTINATION lib
        )
else()
        install(TARGETS OTBSupplFilters LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif()
