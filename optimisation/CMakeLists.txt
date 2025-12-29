PROJECT(opt)


INCLUDE_DIRECTORIES(
    ${LPSOLVE_INCLUDE_DIR}
    ${SQLite_SOURCE_DIR}
    ${opt_SOURCE_DIR}
    ${QT5_INCLUDE_DIRS}
    ${OTB_INCLUDE_DIRS}
    ${OTB_DIR}/lib
    ${OTB_DIR}/bin
    ${OTBSupplCore_SOURCE_DIR}
    ${OTBSupplCore_BINARY_DIR}
    ${shared_SOURCE_DIR}
    ${shared_BINARY_DIR}
    ${MPI_CXX_INCLUDE_DIRS}
    ${NMQt_SOURCE_DIR}
    ${NMQt_SOURCE_DIR}/drivers/sqlite
    ${NMVtk_SOURCE_DIR}
    ${mfw_core_BINARY_DIR}
)

LINK_DIRECTORIES(
    ${LIBRARY_OUTPUT_PATH}
        #${QT5_LINK_DIRS}
    ${opt_BINARY_DIR}
    ${NCXX4_LIB_DIR}
    ${NETCDF_LIB_DIR}
    ${mfw_core_SOURCE_DIR}
    ${VTK_BINARY_DIR}
    ${QT5_LINK_DIRS}
    ${LPSOLVE_LIB_DIR}
    ${SPATIALITE_LIB_DIR}
    ${SPATIALITE_LIBLIB_DIR}
    ${shared_BINARY_DIR}
    ${LIBRARY_OUTPUT_PATH}
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

if (win32)
    set(MOSRA_LINK_LIBS 
        Qt5::Core Qt5::Sql
        ${LPSOLVE_LIBRARY}
        ${NCXX4_LIBRARY}
        ${NETCDF_LIBRARY}
        NMShared
        NMQt
        VTK::CommonCore VTK::CommonDataModel VTK::IOCore VTK::IOSQL VTK::IOLegacy VTK::IOXML
        SQLite
        NMOTBSupplCoreRAT 
        NMOTBSupplCoreIO
        NMOTBSupplCoreReader
        NMOTBSupplCoreWriter
        msvcrt
        KERNEL32
        VCRUNTIME140
        api-ms-win-crt-heap-l1-1-0
        #api-ms-win-crt-math-l1-1-0
        api-ms-win-crt-string-l1-1-0
        api-ms-win-crt-stdio-l1-1-0
        api-ms-win-crt-convert-l1-1-0
        #api-ms-win-crt-filesystem-l1-1-0
        api-ms-win-crt-runtime-l1-1-0
        api-ms-win-crt-utility-l1-1-0
        api-ms-win-crt-time-l1-1-0
        api-ms-win-crt-environment-l1-1-0        
    )
else()
    set(MOSRA_LINK_LIBS 
        Qt5::Core Qt5::Sql
        ${LPSOLVE_LIBRARY}
        ${NCXX4_LIBRARY}
        ${NETCDF_LIBRARY}
        NMShared
        NMQt
        VTK::CommonCore VTK::CommonDataModel VTK::IOCore VTK::IOSQL VTK::IOLegacy VTK::IOXML
        SQLite
        NMOTBSupplCoreRAT 
        NMOTBSupplCoreIO
        NMOTBSupplCoreReader
        NMOTBSupplCoreWriter
    )
endif()

if (MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND MOSRA_LINK_LIBS ucrtd)
endif()

add_definitions(-DNMMosra_EXPORTS)
ADD_LIBRARY(NMMosra SHARED ${OPT_CPP})
generate_export_header(NMMosra)
TARGET_LINK_LIBRARIES(NMMosra ${MOSRA_LINK_LIBS})
    

ADD_DEPENDENCIES(NMMosra NMShared NMQt         
        NMOTBSupplCoreRAT 
        NMOTBSupplCoreIO
        NMOTBSupplCoreReader
        NMOTBSupplCoreWriter
)

if (WIN32)
    install(
        TARGETS NMMosra
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        )
else()
    install(TARGETS NMMosra LIBRARY DESTINATION lib)
endif()

install(FILES ${OPT_HEADER} DESTINATION include)

