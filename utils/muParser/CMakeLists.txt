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

ADD_LIBRARY(MuParser SHARED ${UTILS_PARSER})

set_target_properties(MuParser PROPERTIES
                        LINKER_LANGUAGE CXX)

install(TARGETS MuParser LIBRARY DESTINATION lib)
install(FILES ${UTILS_PARSER_HEADER} DESTINATION include)
