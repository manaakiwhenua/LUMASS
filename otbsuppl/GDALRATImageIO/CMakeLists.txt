PROJECT(GDALRATImageIO)

INCLUDE_DIRECTORIES(
	${GDALRATImageIO_SOURCE_DIR}
        ${GDALRATImageIO_BINARY_DIR}
	${shared_SOURCE_DIR}
        #${rimageio_SOURCE_DIR}
        #${RASDAMAN_SOURCE_DIR}/applications/rasgeo
        ${OTB_DIR}/Code/Common
	${OTB_DIR}/Code/IO
	${OTB_DIR}/Utilities/ITK/Code/Common
    ${SQLite_SOURCE_DIR}
    ${SPATIALITE_INCLUDE_DIR}
) 

LINK_DIRECTORIES(
	${OTB_DIR}/bin
	${OTB_DIR}/bin/${CMAKE_BUILD_TYPE}
	${OTB_DIR}/lib
    #${SQLITE3_INCLUDE_DIR}
    ${OSGEO4W_ROOT}/lib
    ${SQLite_BINARY_DIR}/lib
    ${SPATIALITE_LIB_DIR}
    ${SPATIALITE_LIBLIB_DIR}
)                                                         


# list of project source files
file(GLOB GDALRATIO 
	${GDALRATImageIO_SOURCE_DIR}/*.cxx
	${GDALRATImageIO_SOURCE_DIR}/*.txx
)
# list of project header files
file(GLOB GDALRATIO_HEADER
	${GDALRATImageIO_SOURCE_DIR}/otbGDALRATImageFileReader.h
	${GDALRATImageIO_SOURCE_DIR}/*.h
	${GDALRATImageIO_BINARY_DIR}/*.h
)

add_definitions(-DOTBGDALRATImageIO_EXPORTS)

include(GenerateExportHeader)
# define build target: shared library 
ADD_LIBRARY(OTBGDALRATImageIO SHARED ${GDALRATIO})
set_property(TARGET OTBGDALRATImageIO PROPERTY LINKER_LANGUAGE CXX)

#set_property(TARGET OTBGDALRATImageIO PROPERTY POSITION_INDEPENDENT_CODE 1)

generate_export_header(OTBGDALRATImageIO)

if(LUMASS_OTB_VERSION VERSION_LESS "5.0.0")
	message(STATUS "OTB ${OTB_VERSION_STRING} - OTBGDALRATImageIO: linking OTBIO ... ")
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
	message(STATUS "OTB ${OTB_VERSION_STRING} - OTBGDALRATImageIO: linking OTBIOGDAL ... ")
		if(WIN32)
                        TARGET_LINK_LIBRARIES(OTBGDALRATImageIO
                            ${GDAL_LIBRARY}
                            SQLite
                            lwgeom_i geos_c freexl_i libxml2 proj_i iconv
                            spatialite4
                            OTBCommon OTBIOGDAL
			)
		else()
			TARGET_LINK_LIBRARIES(OTBGDALRATImageIO
					${GDAL_LIBRARY}
					SQLite spatialite
                    OTBCommon OTBIOGDAL
			)
		endif()
endif()

ADD_DEPENDENCIES(OTBGDALRATImageIO SQLite)

# define where to install what
install(FILES ${GDALRATIO_HEADER} DESTINATION include)
if(WIN32)
	install(
		TARGETS OTBGDALRATImageIO 
		LIBRARY DESTINATION lib
		RUNTIME DESTINATION lib
	)
else()
	install(TARGETS OTBGDALRATImageIO LIBRARY DESTINATION lib)
endif()



