PROJECT(OTBSupplCore)

INCLUDE_DIRECTORIES(
    ${NCXX4_INCLUDE_DIR}
    ${NETCDF_INCLUDE_DIR}
    ${NETCDFIO_SOURCE_DIR}
    ${NETCDFIO_BINARY_DIR}
    ${shared_SOURCE_DIR}
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
    ${OTBSupplCore_BINARY_DIR}
    ${OTBSupplCore_SOURCE_DIR}
    ${MPI_CXX_INCLUDE_DIRS}
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
file(GLOB OTBSupplCore_CXX
        ${OTBSupplCore_SOURCE_DIR}/otbAttributeTable.cxx
        ${OTBSupplCore_SOURCE_DIR}/otbNMImageReader.cxx
        ${OTBSupplCore_SOURCE_DIR}/otbNMTableReader.cxx
        ${OTBSupplCore_SOURCE_DIR}/otbRAMTable.cxx
        ${OTBSupplCore_SOURCE_DIR}/otbSQLiteTable.cxx
        ${OTBSupplCore_SOURCE_DIR}/otbStreamingRATImageFileWriter.cxx
)

# list of project header files
file(GLOB OTBSupplCore_HEADER
    ${OTBSupplCore_SOURCE_DIR}/otbAttributeTable.h
    ${OTBSupplCore_SOURCE_DIR}/otbNMImageReader.h
    ${OTBSupplCore_SOURCE_DIR}/otbNMTableReader.h
    ${OTBSupplCore_SOURCE_DIR}/otbRAMTable.h
    ${OTBSupplCore_SOURCE_DIR}/otbSQLiteTable.h
    ${OTBSupplCore_SOURCE_DIR}/otbStreamingRATImageFileWriter.h
    ${OTBSupplCore_BINARY_DIR}/*.h
)

add_definitions(-DNMOTBSupplCore_EXPORTS -DNM_PROC_LOG)
include(GenerateExportHeader)

# define build target: shared library
ADD_LIBRARY(NMOTBSupplCore SHARED ${OTBSupplCore_CXX})
set_property(TARGET NMOTBSupplCore PROPERTY LINKER_LANGUAGE CXX)

generate_export_header(NMOTBSupplCore)

target_link_libraries(NMOTBSupplCore
    ${NETCDF_LIBRARY}
    SQLite
    spatialite
    OTBCommon
    OTBImageBase
    ${MPI_CXX_LIBRARIES}
)

ADD_DEPENDENCIES(NMOTBSupplCore SQLite)

if(WIN32)
        install(
                TARGETS NMOTBSupplCore
                LIBRARY DESTINATION lib
                RUNTIME DESTINATION lib
        )
else()
        install(TARGETS NMOTBSupplCore LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif()



FILE(GLOB SHARED_HEADER
        ${shared_SOURCE_DIR}/*.h
)

file(COPY
        ${SHARED_HEADER}
        DESTINATION ${shared_BINARY_DIR}
)

file(COPY ${OTBSupplCore_HEADER}
        DESTINATION ${OTBSupplCore_BINARY_DIR}
)

install(FILES ${SHARED_HEADER} DESTINATION include)
install(FILES ${OTBSupplCore_HEADER} DESTINATION include)
