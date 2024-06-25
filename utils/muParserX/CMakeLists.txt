PROJECT(muParserX)

#INCLUDE_DIRECTORIES(
#	${otbsuppl_SOURCE_DIR}/GDALRATImageIO
#	${lumass_SOURCE_DIR}/shared
#	${OTB_DIR}/Code/Common
#	${OTB_DIR}/Utilities/ITK/Utilities/vxl/core/vnl
#)

file(GLOB UTILS_PARSERX
    ${muParserX_SOURCE_DIR}/*.cpp
)

file(GLOB UTILS_PARSERX_HEADER
    ${muParserX_SOURCE_DIR}/*.h
)

#ADD_LIBRARY(MuParser SHARED ${UTILS_PARSER})


#install(TARGETS MuParser LIBRARY DESTINATION lib)
#install(FILES ${UTILS_PARSER_HEADER} DESTINATION include)
