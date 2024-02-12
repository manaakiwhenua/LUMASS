PROJECT(opt)


INCLUDE_DIRECTORIES(
    ${LPSOLVE_INCLUDE_DIR}
    ${opt_SOURCE_DIR}
    ${QT5_INCLUDE_DIRS}
    ${OTB_INCLUDE_DIRS}
    ${OTB_DIR}/lib
    ${OTB_DIR}/bin
    ${OTBSupplCore_SOURCE_DIR}
    ${OTBSupplCore_BINARY_DIR}
    ${shared_SOURCE_DIR}
    ${MPI_CXX_INCLUDE_DIRS}
    ${NMQt_SOURCE_DIR}
    ${NMQt_SOURCE_DIR}/drivers/sqlite
    ${NMVtk_SOURCE_DIR}
    ${mfw_core_BINARY_DIR}
)

LINK_DIRECTORIES(
    ${LIBRARY_OUTPUT_PATH}
    ${opt_BINARY_DIR}
    ${NCXX4_LIB_DIR}
    ${NETCDF_LIB_DIR}
    ${mfw_core_SOURCE_DIR}
)


file(GLOB OPT_CPP
   ${NMVtk_SOURCE_DIR}/NMvtkDelimitedTextWriter.cxx
   ${opt_SOURCE_DIR}/*.cpp
)

file(GLOB OPT_HEADER
    ${NMVtk_SOURCE_DIR}/NMvtkDelimitedTextWriter.h
   ${opt_SOURCE_DIR}/NMMosra.h
   ${opt_SOURCE_DIR}/MOSORunnable.h
)


set(CMAKE_AUTOMOC YES)
include(GenerateExportHeader)

ADD_LIBRARY(NMMosra SHARED ${OPT_CPP})
generate_export_header(NMMosra)
TARGET_LINK_LIBRARIES(NMMosra
    Qt5Core Qt5Sql
    ${LPSOLVE_LIBRARY}
    ${NCXX4_LIBRARY}
    ${NETCDF_LIBRARY}
    NMQt
    NMModFrameCore
    VTK::CommonCore VTK::CommonDataModel VTK::IOCore VTK::IOSQL VTK::IOLegacy VTK::IOXML
    SQLite
    NMOTBSupplCore
)

if (WIN32)
    install(
        TARGETS NMMosra
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION lib
        )
else()
    install(TARGETS NMMosra LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif()

install(FILES ${OPT_HEADER} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

