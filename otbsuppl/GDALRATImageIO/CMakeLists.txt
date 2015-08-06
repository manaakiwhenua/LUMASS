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
) 

LINK_DIRECTORIES(
	${OTB_DIR}/bin
	${OTB_DIR}/bin/${CMAKE_BUILD_TYPE}
	${OTB_DIR}/lib
)                                                        

#message(STATUS "Looking for OTB libs in: ${OTB_DIR}/bin and ${OTB_DIR}/bin/Release and ${OTB_DIR}/bin/Deubg ... ")

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
	TARGET_LINK_LIBRARIES(OTBGDALRATImageIO ${GDAL_LIBRARY} OTBCommon OTBIO)
else()
	message(STATUS "OTB ${OTB_VERSION_STRING} - OTBGDALRATImageIO: linking OTBIOGDAL ... ")
	TARGET_LINK_LIBRARIES(OTBGDALRATImageIO ${GDAL_LIBRARY} 
		OTBCommon OTBIOGDAL)
endif()

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



