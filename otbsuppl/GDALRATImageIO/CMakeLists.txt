PROJECT(GDALRATImageIO)

INCLUDE_DIRECTORIES(
	${GDALRATImageIO_SOURCE_DIR}
	${shared_SOURCE_DIR}
        #${rimageio_SOURCE_DIR}
        #${RASDAMAN_SOURCE_DIR}/applications/rasgeo
	${OTB_DIR}/Code/Common
	${OTB_DIR}/Code/IO
	${OTB_DIR}/Utilities/ITK/Code/Common
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
)

# define build target: shared library 
ADD_LIBRARY(OTBGDALRATImageIO SHARED ${GDALRATIO})
set_property(TARGET OTBGDALRATImageIO PROPERTY LINKER_LANGUAGE CXX)
#set_property(TARGET OTBGDALRATImageIO PROPERTY POSITION_INDEPENDENT_CODE 1)
TARGET_LINK_LIBRARIES(OTBGDALRATImageIO ${GDAL_LIBRARY} OTBCommon OTBIO)

# define where to install what
install(FILES ${GDALRATIO_HEADER} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
install(TARGETS OTBGDALRATImageIO LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)



