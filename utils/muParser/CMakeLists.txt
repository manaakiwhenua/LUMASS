PROJECT(muparser)

#INCLUDE_DIRECTORIES(
#	${otbsuppl_SOURCE_DIR}/GDALRATImageIO
#	${lumass_SOURCE_DIR}/shared
#	${OTB_DIR}/Code/Common
#	${OTB_DIR}/Utilities/ITK/Utilities/vxl/core/vnl
#)

file(GLOB UTILS_PARSER
	${muparser_SOURCE_DIR}/*.cpp
)

file(GLOB UTILS_PARSER_HEADER
	${muparser_SOURCE_DIR}/*.h
)

#ADD_LIBRARY(MuParser SHARED ${UTILS_PARSER})


#install(TARGETS MuParser LIBRARY DESTINATION 
#	${CMAKE_INSTALL_PREFIX}/lib)
#install(FILES ${UTILS_PARSER_HEADER} DESTINATION
#	${CMAKE_INSTALL_PREFIX}/include)