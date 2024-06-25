PROJECT(GDALRATImageIO)

INCLUDE_DIRECTORIES(
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${NCXX4_INCLUDE_DIR}
    ${NETCDF_INCLUDE_DIR}
    ${NETCDFIO_SOURCE_DIR}
    ${NETCDFIO_BINARY_DIR}
    ${shared_SOURCE_DIR}
    ${OTBSupplCore_SOURCE_DIR}
    ${OTBSupplCore_BINARY_DIR}
    #${rimageio_SOURCE_DIR}
    #${RASDAMAN_SOURCE_DIR}/applications/rasgeo
    ${ITK_INCLUDE_DIRS}
    ${OTB_INCLUDE_DIRS}
    ${OTB_DIR}/Code/Common
    ${OTB_DIR}/Code/IO
    ${OTB_DIR}/Utilities/ITK/Code/Common
    ${SQLite_SOURCE_DIR}
    ${SPATIALITE_INCLUDE_DIR}
    ${shared_BINARY_DIR}
    ${MPI_CXX_INCLUDE_DIRS}
)

LINK_DIRECTORIES(
    ${ITK_LINK_DIR}
    ${OTB_LINK_DIR}
    ${OTB_DIR}/bin
    ${OTB_DIR}/bin/${CMAKE_BUILD_TYPE}
    ${OTB_DIR}/lib
    ${SQLite_BINARY_DIR}/lib
    ${SPATIALITE_LIB_DIR}
    ${SPATIALITE_LIBLIB_DIR}
    ${NETCDFIO_BINARY_DIR}
    ${NETCDF_LIB_DIR}
    ${NCXX4_LIB_DIR}
    ${OSGEO4W_ROOT}/lib
    ${Boost_LIBRARY_DIRS}
    ${HDF5_ROOT}
)

# list of project source files
file(GLOB GDALRATIO
        ${GDALRATImageIO_SOURCE_DIR}/*.cxx
)

# list of project header files
file(GLOB GDALRATIO_HEADER
        ${GDALRATImageIO_SOURCE_DIR}/*.h
        ${GDALRATImageIO_BINARY_DIR}/*.h
)

add_definitions(-DNMOTBGDALRATImageIO_EXPORTS -DNM_PROC_LOG)

include(GenerateExportHeader)
# define build target: shared library
ADD_LIBRARY(NMOTBGDALRATImageIO SHARED ${GDALRATIO})
set_property(TARGET NMOTBGDALRATImageIO PROPERTY LINKER_LANGUAGE CXX)
generate_export_header(NMOTBGDALRATImageIO)

if(WIN32)
    TARGET_LINK_LIBRARIES(NMOTBGDALRATImageIO
        ${GDAL_LIBRARY}
        #SQLite
        #lwgeom_i geos_c freexl_i libxml2 proj_i iconv zlib
        #spatialite4
        OTBCommon OTBIOGDAL
        #netcdf netcdf-cxx4
        )
else()
    TARGET_LINK_LIBRARIES(NMOTBGDALRATImageIO
        OTBCommon OTBImageBase OTBIOGDAL #OTBCurlAdapters # OTBImageIO
        Boost::filesystem
        NMOTBSupplCore
        curl
        ${NETCDF_LIBRARY}
        ${HDF5_hdf5_LIBRARY_RELEASE} ${HDF5_hdf5_hl_LIBRARY_RELEASE}
        )
endif()

# define where to install what
install(FILES ${GDALRATIO_HEADER} DESTINATION include)
if(WIN32)
        install(
                TARGETS NMOTBGDALRATImageIO
                LIBRARY DESTINATION lib
                RUNTIME DESTINATION lib
        )
else()
        install(TARGETS NMOTBGDALRATImageIO LIBRARY DESTINATION lib)
endif()



