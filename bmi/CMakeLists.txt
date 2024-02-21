PROJECT(bmi)

if (${VTK_VERSION_STRING} VERSION_LESS "8.90")
    #find_package(VTK COMPONENTS
    #    CommonCore
    #    CommonDataModel
    #    IOCore
    #    IOSQL
    #    IOLegacy
    #    IOXML
    #)
    SET(LUMASS_BMI_VTK_LIBRARIES
        vtkCommonCore
        vtkCommonDataModel
        vtkIOCore
        vtkIOSQL
        vtkIOLegacy
        vtkIOXML
    )
    include(${VTK_USE_FILE})
else()
   SET(LUMASS_BMI_VTK_LIBRARIES
       VTK::CommonCore
       VTK::CommonDataModel
       VTK::IOCore
       VTK::IOSQL
       VTK::IOLegacy
       VTK::IOXML
   )
endif()

INCLUDE_DIRECTORIES(
    ${QT5_INCLUDE_DIRS}
    ${opt_SOURCE_DIR}
    ${NMVtk_SOURCE_DIR}
    ${gui_SOURCE_DIR}
    ${shared_SOURCE_DIR}
    ${shared_BINARY_DIR}
    ${lumass_BINARY_DIR}
    ${bmi_BINARY_DIR}
    ${lumass_SOURCE_DIR}
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${OTBSupplCore_SOURCE_DIR}
    ${OTBSupplCore_BINARY_DIR}
    ${filters_SOURCE_DIR}
    ${filters_BINARY_DIR}
    ${rimageio_SOURCE_DIR}
    ${mfw_core_SOURCE_DIR}
    ${mfw_core_BINARY_DIR}
    ${mfw_wrapper_SOURCE_DIR}
    ${mfw_wrapper_BINARY_DIR}
    ${LPSOLVE_INCLUDE_DIR}
    ${OTB_INCLUDE_DIRS}
    ${RASDAMAN_INCLUDE_DIR}
    ${RASDAMAN_INCLUDE_DIR}/raslib
    ${RASDAMAN_SOURCE_DIR}/applications/rasgeo
    ${bmi_SOURCE_DIR}
    ${bmi_SOURCE_DIR}/test
    ${YAML_CPP_INCLUDE_DIR}
    ${Python3_INCLUDE_DIRS}
    ${pybind11_INCLUDE_DIRS}
    #${lumassengine_SOURCE_DIR}
    #${PostgreSQL_INCLUDE_DIRS}
    ${MPI_CXX_INCLUDE_DIRS}
    ${SQLite_SOURCE_DIR}
)

LINK_DIRECTORIES(
    ${QT5_LINK_DIRS}
    ${OTB_LIBRARY_DIRS}
    ${LIBRARY_OUTPUT_PATH}
    ${opt_BINARY_DIR}
    ${bmi_BINARY_DIR}
    #${muparser_BINARY_DIR}
    ${RASDAMAN_LIBRARIES_DIR}
    #${PostgreSQL_LIBRARY_DIRS}
    ${OSGEO4W_ROOT}/lib
    #${SPATIALITE_LIB_DIR}
    ${SPATIALITE_LIBLIB_DIR}
    ${NCXX4_LIB_DIR}
    ${NETCDF_LIB_DIR}
    ${YAML_CPP_LIB_DIR}
    ${Python3_LIBRARIES_DIR}
    ${HDF5_ROOT}
)

# Qt5 requirement
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -fPIC")

file(GLOB BMI_HEADER
    ${bmi_SOURCE_DIR}/*.h
)

file(GLOB BMI_SRC
    #${opt_SOURCE_DIR}/*.cpp
    ${bmi_SOURCE_DIR}/lumassbmi.cpp
    ${bmi_SOURCE_DIR}/NMLumassEngine.cpp
    ${NMVtk_SOURCE_DIR}/NMvtkDelimitedTextWriter.cxx

    ${shared_SOURCE_DIR}/*.cpp
)

file(GLOB PYTHON_BMI_SRC
    ${bmi_SOURCE_DIR}/python*.cpp
    ${shared_SOURCE_DIR}/*.cpp
)

SET(LumassBMI_TEST_FILES
    ${bmi_SOURCE_DIR}/test/LumassBMIConfigTemplate.yaml
)

SET(BMI_LINKLIBS
    Qt5Core Qt5Sql Qt5Xml
    ${NCXX4_LIBRARY}
    ${NETCDF_LIBRARY}
    curl xml2
    ${MPI_CXX_LIBRARIES}
    ${HDF5_hdf5_LIBRARY_RELEASE} ${HDF5_hdf5_hl_LIBRARY_RELEASE}
    NMMosra
    NMOTBSupplFilters
    NMOTBSupplCore
    NMModFrameCore
    ${MFW_WRAPPER_NAMES}
    ${LUMASS_BMI_VTK_LIBRARIES}
    ${LPSOLVE_LIBRARY}
    ${YAML_CPP_LIBRARIES}
)

SET(PYTHON_BMI_LINKLIBS
    Qt5Core
        #NMModFrameCore
    ${Python3_LIBRARIES}
    pybind11::pybind11
    pybind11::embed
    ${YAML_CPP_LIBRARIES}
)

SET(PYTHON_BMI_DEPLIBS
    NMModFrameCore
)


SET(BMI_DEPLIBS
    NMModFrameCore ${MFW_WRAPPER_NAMES}
)

set(CMAKE_AUTOMOC YES)
include(GenerateExportHeader)

# -----------------------------------------
# build the LUMASS BMI interface
ADD_LIBRARY(LumassBMI SHARED ${BMI_SRC})
generate_export_header(LumassBMI)
TARGET_LINK_LIBRARIES(LumassBMI ${BMI_LINKLIBS})
ADD_DEPENDENCIES(LumassBMI ${BMI_DEPLIBS})


if (PYTHONSUPPORT)
    ADD_DEFINITIONS(-DLUMASS_PYTHON)
    ADD_LIBRARY(LumassPythonBMI SHARED ${PYTHON_BMI_SRC})
    generate_export_header(LumassPythonBMI)
    TARGET_LINK_LIBRARIES(LumassPythonBMI ${PYTHON_BMI_LINKLIBS} ${MPI_CXX_LIBRARIES}
)
    #ADD_DEPENDENCIES(LumassPythonBMI ${PYTHON_BMI_DEPLIBS})
endif()

if (${VTK_VERSION_STRING} VERSION_GREATER_EQUAL "8.90")
    vtk_module_autoinit(TARGETS LumassBMI LumassPythonBMI MODULES ${LUMASS_BMI_VTK_LIBRARIES})
endif()


# -----------------------------------------
# add the test application
#set(BMITEST_LINK_LIBS dl)
#if(PYTHONSUPPORT)
#    list(APPEND BMITEST_LINK_LIBS pybind11::embed)
#endif()


#ADD_EXECUTABLE(LumassBMITest ${bmi_SOURCE_DIR}/test/LumassBMITest.cpp)
#if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#    TARGET_LINK_LIBRARIES(LumassBMITest dl)
#endif()

SET(LUMASS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})

#if (WIN32)
        #configure_file("${bmi_SOURCE_DIR}/test/LumassEnv.cmd.in"
    #           "${bmi_BINARY_DIR}/test/LumassEnv.cmd" @ONLY)
        #LIST(APPEND LumassBMI_TEST_FILES ${LumassBMITest_BINARY_DIR}/LumassEnv.cmd)
#endif()

#install(TARGETS LumassBMITest DESTINATION test)
#install(FILES ${LumassBMI_TEST_FILES} DESTINATION test)

# -------------------------------------------
# INSTALL LIBRARY
SET(BMI_INSTALL_TARGETS LumassBMI)
if (PYTHONSUPPORT)
    list(APPEND BMI_INSTALL_TARGETS LumassPythonBMI)
endif()

#install(FILES ${BMI_HEADER} DESTINATION include)
if (WIN32)
    install (TARGETS ${BMI_INSTALL_TARGETS}
             LIBRARY DESTINATION lib
             RUNTIME DESTINATION lib
     )
else()
    install(TARGETS ${BMI_INSTALL_TARGETS} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif()


ADD_SUBDIRECTORY(test ${bmi_BINARY_DIR}/test)
