PROJECT(OTBSupplCore)

INCLUDE_DIRECTORIES(
    ${NCXX4_INCLUDE_DIR}
    ${NETCDF_INCLUDE_DIR}
    ${NETCDFIO_SOURCE_DIR}
    ${NETCDFIO_BINARY_DIR}
    ${shared_SOURCE_DIR}
    ${GDALRATImageIO_SOURCE_DIR}
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
    ${mfw_core_BINARY_DIR}
    ${MPI_CXX_INCLUDE_DIRS}
    ${GDALRATImageIO_SOURCE_DIR}
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
    ${OTBSupplCore_BINARY_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${NETCDF_LIB_DIR}
    ${NCXX4_LIB_DIR}
    ${OSGEO4W_ROOT}/lib
    ${Boost_LIBRARY_DIRS}
    ${HDF5_ROOT}
    ${LIBRARY_OUTPUT_PATH}
)

if (MSVC)
    add_compile_options(/bigobj)
endif()

include(GenerateExportHeader)

if (WIN32)
   set(LINKLIBS_WIN32
       ${GDAL_LIBRARY}
       ${NETCDF_LIBRARY}
       ${NCXX4_LIBRARY}
       Boost::filesystem
       SQLite
       spatialite
       libcurl_imp
       geos_c
       freexl_i
       libxml2
       zlib 
       iconv
       librttopo
       proj
       ${OTB_LINK_LIBS}
       ${MPI_CXX_LIBRARIES}
   )
else()
   set(LINKLIBS_WIN32
       ${NCXX4_LIBRARY}
       ${NETCDF_LIBRARY}
       Boost::filesystem
       curl xml2
       SQLite
       spatialite
       geos_c
       OTBCommon
       OTBImageBase
       OTBIOGDAL
       OTBImgageIO
       OTBCurlAdapters
       ${MPI_CXX_LIBRARIES}
       ${HDF5_hdf5_LIBRARY_RELEASE} ${HDF5_hdf5_hl_LIBRARY_RELEASE}
   )
endif()

#############################################################################################
############ NMOTBSupplCoreRAT ##############################################################
#############################################################################################

# list of OTBSupplCoreRAT project source files
file(GLOB OTBSupplCoreRAT_CXX
        ${OTBSupplCore_SOURCE_DIR}/otbAttributeTable.cxx
        ${OTBSupplCore_SOURCE_DIR}/otbNMTableReader.cxx
        ${OTBSupplCore_SOURCE_DIR}/otbRAMTable.cxx
        ${OTBSupplCore_SOURCE_DIR}/otbSQLiteTable.cxx
)

# list of OTBSupplCore project header files
file(GLOB OTBSupplCoreRAT_HEADER
    ${OTBSupplCore_SOURCE_DIR}/otbAttributeTable.h
    ${OTBSupplCore_SOURCE_DIR}/otbNMTableReader.h
    ${OTBSupplCore_SOURCE_DIR}/otbRAMTable.h
    ${OTBSupplCore_SOURCE_DIR}/otbSQLiteTable.h
)

# define build target: shared library
ADD_LIBRARY(NMOTBSupplCoreRAT SHARED ${OTBSupplCoreRAT_CXX})
target_compile_definitions(NMOTBSupplCoreRAT PRIVATE NMOTBSupplCoreRAT_EXPORTS)
set_property(TARGET NMOTBSupplCoreRAT PROPERTY LINKER_LANGUAGE CXX)

generate_export_header(NMOTBSupplCoreRAT)
ADD_DEPENDENCIES(NMOTBSupplCoreRAT SQLite)

#message(STATUS "OTBSupplCore: OTB_LINK_LIBS=${OTB_LINK_LIBS}")

if (WIN32)
   target_link_libraries(NMOTBSupplCoreRAT ${LINKLIBS_WIN32})
   install(
           TARGETS NMOTBSupplCoreRAT
           LIBRARY DESTINATION lib
           RUNTIME DESTINATION bin
   )
else()
    target_link_libraries(NMOTBSupplCoreRAT ${LINKLIBS_LINUX})
    install(TARGETS NMOTBSupplCoreRAT LIBRARY DESTINATION lib)
endif()

file(COPY ${OTBSupplCoreRAT_HEADER}
       DESTINATION ${OTBSupplCore_BINARY_DIR}
)

install(FILES ${OTBSupplCoreRAT_HEADER} DESTINATION include)

#############################################################################################
############ NMOTBSupplCoreIO ###############################################################
#############################################################################################

# list of OTBSupplCoreIO project source files
file(GLOB OTBSupplCoreIO_CXX
        ${otbsuppl_SOURCE_DIR}/GDALRATImageIO/*.cxx
        ${otbsuppl_SOURCE_DIR}/NETCDFIO/*.cxx
)

# list of OTBSupplCoreIO project header files
file(GLOB OTBSupplCoreIO_HEADER
        ${otbsuppl_SOURCE_DIR}/GDALRATImageIO/*.h
        ${otbsuppl_SOURCE_DIR}/NETCDFIO/*.h
)

# define build target: shared library
ADD_LIBRARY(NMOTBSupplCoreIO SHARED ${OTBSupplCoreIO_CXX})
target_compile_definitions(NMOTBSupplCoreIO PRIVATE NMOTBSupplCoreIO_EXPORTS)
set_property(TARGET NMOTBSupplCoreIO PROPERTY LINKER_LANGUAGE CXX)

generate_export_header(NMOTBSupplCoreIO)
ADD_DEPENDENCIES(NMOTBSupplCoreIO NMOTBSupplCoreRAT)

if(WIN32)
   target_link_libraries(NMOTBSupplCoreIO 
        ${LINKLIBS_WIN32} 
        NMOTBSupplCoreRAT
   )

   install(
           TARGETS NMOTBSupplCoreIO
           LIBRARY DESTINATION lib
           RUNTIME DESTINATION bin
   )
else()
   target_link_libraries(NMOTBSupplCoreIO 
        ${LINKLIBS_LINUX}
        NMOTBSupplCoreRAT
   )

   install(TARGETS NMOTBSupplCoreIO LIBRARY DESTINATION lib)
endif()


file(COPY ${OTBSupplCoreIO_HEADER}
       DESTINATION ${OTBSupplCore_BINARY_DIR}
)

install(FILES ${OTBSupplCoreIO_HEADER} DESTINATION include)


#############################################################################################
############ NMOTBSupplCoreReader ###########################################################
#############################################################################################


# list of OTBSupplCore project source files
file(GLOB OTBSupplCoreReader_CXX
        ${OTBSupplCore_SOURCE_DIR}/otbNMImageReader.cxx
)

# list of OTBSupplCore project header files
file(GLOB OTBSupplCoreReader_HEADER
    ${OTBSupplCore_SOURCE_DIR}/otbNMImageReader.h
)

# define build target: shared library
ADD_LIBRARY(NMOTBSupplCoreReader SHARED ${OTBSupplCoreReader_CXX})
target_compile_definitions(NMOTBSupplCoreReader PRIVATE NMOTBSupplCoreReader_EXPORTS)
set_property(TARGET NMOTBSupplCoreReader PROPERTY LINKER_LANGUAGE CXX)

generate_export_header(NMOTBSupplCoreReader)
ADD_DEPENDENCIES(NMOTBSupplCoreReader NMOTBSupplCoreRAT NMOTBSupplCoreIO)

if(WIN32)
   target_link_libraries(NMOTBSupplCoreReader 
        ${LINKLIBS_WIN32}
        NMOTBSupplCoreRAT
        NMOTBSupplCoreIO
   )

   install(
           TARGETS NMOTBSupplCoreReader
           LIBRARY DESTINATION lib
           RUNTIME DESTINATION bin
   )
else()
   target_link_libraries(NMOTBSupplCoreReader 
        ${LINKLIBS_LINUX}
        NMOTBSupplCoreRAT
        NMOTBSupplCoreIO
   )

   install(TARGETS NMOTBSupplCoreReader LIBRARY DESTINATION lib)
endif()

file(COPY ${OTBSupplCoreReader_HEADER}
       DESTINATION ${OTBSupplCore_BINARY_DIR}
)

install(FILES ${OTBSupplCoreReader_HEADER} DESTINATION include)

#############################################################################################
############ NMOTBSupplCoreWriter ###########################################################
#############################################################################################


# list of OTBSupplCoreWriter project source files
file(GLOB OTBSupplCoreWriter_CXX
        ${OTBSupplCore_SOURCE_DIR}/otbStreamingRATImageFileWriter.cxx
)

# list of OTBSupplCoreWriter project header files
file(GLOB OTBSupplCoreWriter_HEADER
    ${OTBSupplCore_SOURCE_DIR}/otbStreamingRATImageFileWriter.h
)

# define build target: shared library
ADD_LIBRARY(NMOTBSupplCoreWriter SHARED ${OTBSupplCoreWriter_CXX})
target_compile_definitions(NMOTBSupplCoreWriter PRIVATE NMOTBSupplCoreWriter_EXPORTS)
set_property(TARGET NMOTBSupplCoreWriter PROPERTY LINKER_LANGUAGE CXX)

generate_export_header(NMOTBSupplCoreWriter)
ADD_DEPENDENCIES(NMOTBSupplCoreWriter NMOTBSupplCoreRAT NMOTBSupplCoreIO)

if(WIN32)
   target_link_libraries(NMOTBSupplCoreWriter 
        ${LINKLIBS_WIN32}
        NMOTBSupplCoreRAT
        NMOTBSupplCoreIO
   )

   install(
           TARGETS NMOTBSupplCoreWriter
           LIBRARY DESTINATION lib
           RUNTIME DESTINATION bin
   )
else()
   target_link_libraries(NMOTBSupplCoreWriter 
        ${LINKLIBS_LINUX}
        NMOTBSupplCoreRAT
        NMOTBSupplCoreIO
   )

   install(TARGETS NMOTBSupplCoreWriter LIBRARY DESTINATION lib)
endif()

file(COPY ${OTBSupplCoreWriter_HEADER}
       DESTINATION ${OTBSupplCore_BINARY_DIR}
)

install(FILES ${OTBSupplCoreWriter_HEADER} DESTINATION include)

#############################################################################################
############ NMItk ##########################################################################
#############################################################################################

file(GLOB NMItk_CXX
    ${OTBSupplCore_SOURCE_DIR}/itkNMConstShapedNeighborhoodIterator.cxx
    ${OTBSupplCore_SOURCE_DIR}/itkNMImageRegionSplitterMaxSize.cxx
)

file(GLOB NMItk_HEADER
    ${OTBSupplCore_SOURCE_DIR}/itkNMConstShapedNeighborhoodIterator.h
    ${OTBSupplCore_SOURCE_DIR}/itkNMConstShapedNeighborhoodIterator.hxx
    ${OTBSupplCore_SOURCE_DIR}/itkNMImageRegionSplitterMaxSize.h
)


if (MSVC)
    add_compile_options(/bigobj)
endif()

ADD_LIBRARY(NMItk SHARED ${NMItk_CXX})
target_compile_definitions(NMItk PRIVATE NMItk_EXPORTS)
set_property(TARGET NMItk PROPERTY LINKER_LANGUAGE CXX)

generate_export_header(NMItk)
target_link_libraries(NMItk
      ${OTB_LINK_LIBS}
)

if (WIN32)
   install(
           TARGETS NMItk
           LIBRARY DESTINATION lib
           RUNTIME DESTINATION bin
   )
else()
   install(TARGETS NMItk LIBRARY DESTINATION lib)
endif()


file(COPY ${NMItk_HEADER}
       DESTINATION ${OTBSupplCore_BINARY_DIR}
)

install(FILES ${NMItk_HEADER} DESTINATION include)

#############################################################################################
############ NMOTBSupplTemplates ############################################################
#############################################################################################

# list of OTBSupplTemplates project source files
file(GLOB OTBSupplTemplates_CXX
        ${OTBSupplCore_SOURCE_DIR}/nmExplicitTemplateInstantiation.cxx
)

# list of OTBSupplCore project header files
file(GLOB OTBSupplTemplates_HEADER
    ${OTBSupplCore_SOURCE_DIR}/nmExplicitTemplateInstantiation.h
)

# define build target: shared library
ADD_LIBRARY(NMOTBSupplTemplates SHARED ${OTBSupplTemplates_CXX})
target_compile_definitions(NMOTBSupplTemplates PRIVATE NMOTBSupplTemplates_EXPORTS)
set_property(TARGET NMOTBSupplTemplates PROPERTY LINKER_LANGUAGE CXX)

generate_export_header(NMOTBSupplTemplates)
#ADD_DEPENDENCIES(NMOTBSupplTemplates)

#message(STATUS "OTBSupplCore: OTB_LINK_LIBS=${OTB_LINK_LIBS}")

if (WIN32)
   target_link_libraries(NMOTBSupplTemplates NMOTBSupplCoreReader ${LINKLIBS_WIN32})
   install(
           TARGETS NMOTBSupplTemplates
           LIBRARY DESTINATION lib
           RUNTIME DESTINATION bin
   )
else()
    target_link_libraries(NMOTBSupplTemplates ${LINKLIBS_LINUX})
    install(TARGETS NMOTBSupplTemplates LIBRARY DESTINATION lib)
endif()

file(COPY ${OTBSupplTemplates_HEADER}
       DESTINATION ${OTBSupplCore_BINARY_DIR}
)

install(FILES ${OTBSupplCoreRAT_HEADER} DESTINATION include)

