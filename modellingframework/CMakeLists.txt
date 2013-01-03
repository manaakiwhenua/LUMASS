PROJECT(mfw)

ADD_SUBDIRECTORY(wrapper ${mfw_BINARY_DIR}/wrapper)

INCLUDE_DIRECTORIES(
    ${PostgreSQL_INCLUDE_DIRS}
    ${RASDAMAN_SOURCE_DIR}/applications/rasgeo
    ${RASDAMAN_INCLUDE_DIR}
    ${RASDAMAN_INCLUDE_DIR}/raslib
    ${VTK_INCLUDE_DIRS}
    ${mfw_BINARY_DIR}
    ${mfw_SOURCE_DIR}
    ${mfw_wrapper_SOURCE_DIR}
    ${shared_SOURCE_DIR}
    ${GDALRATImageIO_SOURCE_DIR}
    ${rimageio_SOURCE_DIR}
    ${filters_SOURCE_DIR}
    ${gui_SOURCE_DIR}
)

LINK_DIRECTORIES(
    ${LIBRARY_OUTPUT_PATH}
)

file(GLOB MFW_CPP
   ${mfw_SOURCE_DIR}/*.cpp
   ${mfw_wrapper_SOURCE_DIR}/*.cpp
   ${mfw_wrapper_SOURCE_DIR}/*.cxx
)

file(GLOB MFW_H
   ${mfw_SOURCE_DIR}/*.h
   ${mfw_wrapper_SOURCE_DIR}/*.h
)

QT4_WRAP_CPP(MFW_MOC_CPP ${MFW_H})

IF(RASSUPPORT)
    add_definitions(-DBUILD_RASSUPPORT)
ENDIF()

ADD_LIBRARY(NMModFrame SHARED ${MFW_CPP} ${MFW_MOC_CPP})
TARGET_LINK_LIBRARIES(NMModFrame OTBSupplFilters)
ADD_DEPENDENCIES(NMModFrame OTBSupplFilters)

install(FILES ${MFW_H} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
install(TARGETS NMModFrame LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)












