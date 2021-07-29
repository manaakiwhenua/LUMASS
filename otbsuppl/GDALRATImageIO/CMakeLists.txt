PROJECT(GDALRATImageIO)

INCLUDE_DIRECTORIES(
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${NCXX4_INCLUDE_DIR}
    ${NETCDF_INCLUDE_DIR}
    ${NETCDFIO_SOURCE_DIR}
    ${NETCDFIO_BINARY_DIR}
    ${shared_SOURCE_DIR}
    ${otbsuppl_core_SOURCE_DIR}
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
)

# list of project source files
file(GLOB GDALRATIO
        ${GDALRATImageIO_SOURCE_DIR}/*.cxx
        ${GDALRATImageIO_SOURCE_DIR}/*.txx
)

# list of project header files
file(GLOB GDALRATIO_HEADER
        ${GDALRATImageIO_SOURCE_DIR}/*.h
        ${GDALRATImageIO_BINARY_DIR}/*.h
)

list(REMOVE_ITEM GDALRATIO ${GDALRATImageIO_SOURCE_DIR}/otbGDALRATImageFileReader.txx)
list(REMOVE_ITEM GDALRATIO_HEADER ${GDALRATImageIO_SOURCE_DIR}/otbGDALRATImageFileReader.h)

add_definitions(-DOTBGDALRATImageIO_EXPORTS -DNM_PROC_LOG)

include(GenerateExportHeader)
# define build target: shared library
ADD_LIBRARY(OTBGDALRATImageIO SHARED ${GDALRATIO})
set_property(TARGET OTBGDALRATImageIO PROPERTY LINKER_LANGUAGE CXX)
#set_property(TARGET OTBGDALRATImageIO PROPERTY LINK_DIRECTORIES
#target_link_directories(OTBGDALRATImageIO PUBLIC
#    ${ITK_LINK_DIR}
#    ${OTB_LINK_DIR}
#    ${OTB_DIR}/bin
#    ${OTB_DIR}/bin/${CMAKE_BUILD_TYPE}
#    ${OTB_DIR}/lib
#    ${SQLite_BINARY_DIR}/lib
#    ${SPATIALITE_LIB_DIR}
#    ${SPATIALITE_LIBLIB_DIR}
#    ${NETCDFIO_BINARY_DIR}
#    ${NETCDF_LIB_DIR}
#    ${NCXX4_LIB_DIR}
#    ${OSGEO4W_ROOT}/lib
#)


#set_property(TARGET OTBGDALRATImageIO PROPERTY POSITION_INDEPENDENT_CODE 1)

generate_export_header(OTBGDALRATImageIO)

if(LUMASS_OTB_VERSION VERSION_LESS "5.0.0")
        message(STATUS "OTB ${LUMASS_OTB_VERSION} - OTBGDALRATImageIO: linking OTBIO ... ")
        if(WIN32)
                TARGET_LINK_LIBRARIES(OTBGDALRATImageIO
                        ${GDAL_LIBRARY}
                        SQLite
                        lwgeom_i geos_c freexl_i libxml2 proj_i iconv
                        spatialite4
                        OTBCommon OTBIO
                        )
        else()
                TARGET_LINK_LIBRARIES(OTBGDALRATImageIO
                        ${GDAL_LIBRARY}
                        SQLite spatialite
                        OTBCommon OTBIO
                        )
                endif()
else()
    message(STATUS "OTB ${LUMASS_OTB_VERSION} - OTBGDALRATImageIO: linking OTBIOGDAL ... ")
    if(WIN32)
        TARGET_LINK_LIBRARIES(OTBGDALRATImageIO
            ${GDAL_LIBRARY}
            SQLite
            lwgeom_i geos_c freexl_i libxml2 proj_i iconv zlib
            spatialite4
            OTBCommon OTBIOGDAL
            #netcdf netcdf-cxx4
            )
    else()
        TARGET_LINK_LIBRARIES(OTBGDALRATImageIO
            ${GDAL_LIBRARY}
            SQLite spatialite
            #/usr/lib/libITKCommon-4.13.so /usr/lib/libITKIOImageBase-4.13.so
            OTBCommon OTBIOGDAL #OTBImageBase OTBImageIO OTBCurlAdapters
            Boost::filesystem
            #${OTB_LIBRARIES}
            #${OTB_LINK_LIBS}
            #${ITK_LIBRARIES}
            #${NETCDF_LIBRARY} ${NCXX4_LIBRARY}
            #/usr/lib/x86_64-linux-gnu/libnetcdf.so  /opt/netcdf-cxx4/install/lib/libnetcdf-cxx4.so
            )
    endif()
endif()

ADD_DEPENDENCIES(OTBGDALRATImageIO SQLite)

# define where to install what
#install(FILES ${GDALRATIO_HEADER} DESTINATION include)
if(WIN32)
        install(
                TARGETS OTBGDALRATImageIO
                LIBRARY DESTINATION lib
                RUNTIME DESTINATION lib
        )
else()
        install(TARGETS OTBGDALRATImageIO LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif()



