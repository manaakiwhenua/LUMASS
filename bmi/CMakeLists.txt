PROJECT(bmi)

find_package(VTK COMPONENTS
vtkCommonCore
vtkCommonDataModel
vtkIOCore
vtkIOSQL
vtkIOLegacy
vtkIOXML
)

include(${VTK_USE_FILE})

INCLUDE_DIRECTORIES(
    ${QT5_INCLUDE_DIRS}
    ${opt_SOURCE_DIR}
    ${NMVtk_SOURCE_DIR}
    ${gui_SOURCE_DIR}
    ${shared_SOURCE_DIR}
    ${shared_BINARY_DIR}
    ${lumass_BINARY_DIR}
    ${lumass_SOURCE_DIR}
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${otbsuppl_core_SOURCE_DIR}
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
    ${YAML_HEADERDIR}
    #${lumassengine_SOURCE_DIR}
    #${PostgreSQL_INCLUDE_DIRS}
)

LINK_DIRECTORIES(
    ${QT5_LINK_DIRS}
    ${OTB_LIBRARY_DIRS}
    ${LIBRARY_OUTPUT_PATH}
    ${bmi_BINARY_DIR}
    ${muparser_BINARY_DIR}
    ${RASDAMAN_LIBRARIES_DIR}
    #${PostgreSQL_LIBRARY_DIRS}
    ${OSGEO4W_ROOT}/lib
    #${SPATIALITE_LIB_DIR}
    ${SPATIALITE_LIBLIB_DIR}
    ${YAML_LIBDIR}
)

# Qt5 requirement
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -fPIC")

file(GLOB BMI_HEADER
    ${bmi_SOURCE_DIR}/*.h
)

file(GLOB BMI_SRC
    #${lumassengine_SOURCE_DIR}/lumassengine.cxx
    ${opt_SOURCE_DIR}/*.cpp
    ${bmi_SOURCE_DIR}/*.cpp
    ${NMVtk_SOURCE_DIR}/NMvtkDelimitedTextWriter.cxx

    ${shared_SOURCE_DIR}/*.cpp
)

SET(LumassBMI_TEST_FILES
    ${bmi_SOURCE_DIR}/test/LumassBMIConfigTemplate.yaml
)

SET(BMI_LINKLIBS
    Qt5Core Qt5Sql Qt5Xml
    NMModFrameCore
    ${MFW_WRAPPER_NAMES}
    ${VTK_LIBRARIES}
    ${LPSOLVE_LIBRARY}
)

if (WIN32)
	LIST(APPEND BMI_LINKLIBS libyaml-cppmd)
else()
    LIST(APPEND BMI_LINKLIBS dl yaml-cpp)
endif()


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

# -----------------------------------------
# add the test application
ADD_EXECUTABLE(LumassBMITest ${bmi_SOURCE_DIR}/test/LumassBMITest.cpp)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	TARGET_LINK_LIBRARIES(LumassBMITest dl)
endif()

SET(LUMASS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})

#if (WIN32)
	#configure_file("${bmi_SOURCE_DIR}/test/LumassEnv.cmd.in"
    #           "${bmi_BINARY_DIR}/test/LumassEnv.cmd" @ONLY)
	#LIST(APPEND LumassBMI_TEST_FILES ${LumassBMITest_BINARY_DIR}/LumassEnv.cmd)
#endif()

install(TARGETS LumassBMITest DESTINATION test)
install(FILES ${LumassBMI_TEST_FILES} DESTINATION test)

# -------------------------------------------
# INSTALL LIBRARY
install(FILES ${BMI_HEADER} DESTINATION include)
if (WIN32)
    install (TARGETS LumassBMI
             LIBRARY DESTINATION lib
             RUNTIME DESTINATION lib
    )
else()
    install(TARGETS LumassBMI LIBRARY DESTINATION lib)
endif()


