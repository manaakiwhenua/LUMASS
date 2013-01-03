PROJECT(filters)

INCLUDE_DIRECTORIES(
	${GDALRATImageIO_SOURCE_DIR}
	${filters_SOURCE_DIR}
	${shared_SOURCE_DIR}
	${OTB_DIR}/Code/Common
	${muparser_SOURCE_DIR}
	${utils_SOURCE_DIR}
	${OTB_DIR}/Utilities/ITK/Utilities/vxl/core/vnl
)

file(GLOB OTBSUPPL_FILTERS
	${filters_SOURCE_DIR}/*.txx
	${filters_SOURCE_DIR}/*.cxx
	${muparser_SOURCE_DIR}/*.cpp
)

file(GLOB OTBSUPPL_HEADER
	${filters_SOURCE_DIR}/*.h
)

ADD_LIBRARY(OTBSupplFilters SHARED ${OTBSUPPL_FILTERS})
set_target_properties(OTBSupplFilters PROPERTIES  
                      	LINKER_LANGUAGE CXX)

install(TARGETS OTBSupplFilters LIBRARY DESTINATION 
	${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${OTBSUPPL_HEADER} DESTINATION
	${CMAKE_INSTALL_PREFIX}/include)