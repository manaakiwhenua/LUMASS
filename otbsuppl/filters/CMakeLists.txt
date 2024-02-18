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
    ${OTB_INCLUDE_DIRS}
    ${muparser_SOURCE_DIR}
    ${muParserX_SOURCE_DIR}
    ${OTBSupplCore_SOURCE_DIR}
    ${OTBSupplCore_BINARY_DIR}
    ${utils_SOURCE_DIR}
    ${OTB_DIR}/Utilities/ITK/Utilities/vxl/core/vnl
    ${lumass_SOURCE_DIR}/utils/ITK
    ${lumass_SOURCE_DIR}
    #${TCADB_INCLUDE_DIR}
    ${PYBIND11_INCLUDE_DIR}
    ${MPI_CXX_INCLUDE_DIRS}
    ${opt_SOURCE_DIR}
)

LINK_DIRECTORIES(
    ${OTB_BINARY_DIR}
    ${OTB_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    ${OTB_DIR}/bin
    ${OTB_DIR}/bin/${CMAKE_BUILD_TYPE}
    ${OTB_DIR}/lib
    ${SPATIALITE_LIB_DIR}
    ${SPATIALITE_LIBLIB_DIR}
    ${NETCDF_LIB_DIR}


    #${TCADB_LIB_DIR}
)

file(GLOB OTBSUPPL_FILTERS
    ${filters_SOURCE_DIR}/*.txx
    ${filters_SOURCE_DIR}/*.cxx
    ${muparser_SOURCE_DIR}/*.cpp
    ${muParserX_SOURCE_DIR}/*.cpp
    ${OTBSupplCore_SOURCE_DIR}/*.cxx
    ${OTBSupplCore_SOURCE_DIR}/*.hxx
)

file(GLOB OTBSUPPL_HEADER
    ${filters_SOURCE_DIR}/*.h
    ${filters_BINARY_DIR}/*.h
)

include(GenerateExportHeader)

add_definitions(-DNM_PROC_LOG)

if(WIN32)
        add_definitions(-DOTBGDALRATIMAGEIO_STATIC_DEFINE)
        ADD_LIBRARY(NMOTBSupplFilters STATIC ${OTBSUPPL_FILTERS})
else(WIN32)
        ADD_LIBRARY(NMOTBSupplFilters SHARED ${OTBSUPPL_FILTERS})
endif(WIN32)

set_target_properties(NMOTBSupplFilters PROPERTIES
                        LINKER_LANGUAGE CXX)

generate_export_header(NMOTBSupplFilters)

if(WIN32)
    target_link_libraries(NMOTBSupplFilters #OTBCommon OTBIOGDAL
    #OTBExtendedFilename OTBCurlAdapters OTBImageIO OTBImageBase
    #OTBMathParser
    #OTBStreaming
    NMOTBGDALRATImageIO
    Qt5Core Qt5Qml
    #tokyocabinet
    )
else()
    target_link_libraries(NMOTBSupplFilters #OTBCommon OTBIOGDAL
    ${MPI_CXX_LIBRARIES}
    ${NETCDF_LIBRARY}
    NMOTBGDALRATImageIO
    Qt5Core Qt5Qml
    )
endif()

add_dependencies(NMOTBSupplFilters NMOTBGDALRATImageIO)

install(FILES ${OTBSUPPL_HEADER} DESTINATION include)
if(WIN32)
        install(TARGETS NMOTBSupplFilters
                #LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib
                RUNTIME DESTINATION lib
        )
else()
        install(TARGETS NMOTBSupplFilters LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif()
