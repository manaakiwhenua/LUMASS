PROJECT(filters)

INCLUDE_DIRECTORIES(
	${otbsuppl_SOURCE_DIR}/GDALRATImageIO
	${otbsuppl_SOURCE_DIR}/../shared
	${OTB_DIR}/Code/Common
	${OTB_DIR}/Utilities/ITK/Utilities/vxl/core/vnl
)

file(GLOB OTBSUPPL_FILTERS
	${filters_SOURCE_DIR}/*.txx
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