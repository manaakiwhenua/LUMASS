PROJECT(gui)

if (${VTK_VERSION_STRING} VERSION_LESS "8.9.0")

    add_definitions(-DVTK_OPENGL2)
    set(DVTKOGL2 "yes")
    set(LUMASS_GUI_VTK_LIBRARIES
        vtkCommonDataModel
        vtkCommonCore
        vtkFiltersGeneral
        vtkRenderingCore
        vtkRenderingAnnotation
        vtkRenderingImage
        vtkRenderingOpenGL2
        #vtkRenderingLOD
        vtkGUISupportQt
        vtkImagingStatistics
        vtkInteractionWidgets
        vtkInteractionStyle
        vtkRenderingFreeType
        vtkIOCore
        vtkIOExport
        vtkIOInfovis
        vtkIOSQL
        vtkIOLegacy
        vtkIOXML
        vtkIOGDAL
        vtkRenderingContextOpenGL2
        vtkViewsCore
        vtkViewsContext2D
        vtkChartsCore
    )
    include(${VTK_USE_FILE})

else()

    add_definitions(-DVTK_OPENGL2)
    set(DVTKOGL2 "yes")
    set(LUMASS_GUI_VTK_LIBRARIES
        VTK::CommonDataModel
        VTK::CommonCore
        VTK::FiltersGeneral
        VTK::FiltersExtraction
        VTK::FiltersGeometry
        VTK::FiltersHybrid
        VTK::RenderingCore
        VTK::RenderingAnnotation
        VTK::RenderingImage
        VTK::RenderingOpenGL2
        VTK::GUISupportQt
        VTK::ImagingStatistics
        VTK::InteractionWidgets
        VTK::InteractionStyle
        VTK::RenderingFreeType
        VTK::ImagingSources
        VTK::IOCore
        VTK::IOExport
        VTK::IOInfovis
        VTK::IOSQL
        VTK::IOLegacy
        VTK::IOXML
        VTK::IOGDAL
        VTK::IOFFMPEG
        VTK::RenderingContextOpenGL2
        VTK::ViewsCore
        VTK::ViewsContext2D
        VTK::ChartsCore
    )

endif()


INCLUDE_DIRECTORIES(
    ${QT5_INCLUDE_DIRS}
    ${QT5CORE_PRIVATE_DIR}
    ${QT5CORE_PRIVATE_DIR}/..
    ${QT5SQL_INCLUDE_DIR}
    #${QT5WIDGETS_INCLUDE_DIR}
    ${NMQT5SQL_PRIVATE_DIR}
    ${NMQT5SQL_PRIVATE_DIR}/..
    ${NMQT5WIDGETS_PRIVATE_DIR}
    ${NMQT5WIDGETS_PRIVATE_DIR}/..
    ${NMQT5GUI_PRIVATE_DIR}
    ${NMQT5GUI_PRIVATE_DIR}/..
    ${opt_SOURCE_DIR}
    ${lumass_BINARY_DIR}
    ${gui_BINARY_DIR}
    ${utils_SOURCE_DIR}/QtPropertyBrowser/src
    ${NMQSQLite_SRC_DIR}
    ${NMVtk_SOURCE_DIR}
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${NETCDFIO_SOURCE_DIR}
    ${gui_SOURCE_DIR}
    ${filters_SOURCE_DIR}
    ${OTBSupplCore_SOURCE_DIR}
    ${OTBSupplCore_BINARY_DIR}
    ${filters_BINARY_DIR}
    ${shared_SOURCE_DIR}
    ${shared_BINARY_DIR}
    ${rimageio_SOURCE_DIR}
    ${mfw_SOURCE_DIR}
    ${mfw_BINARY_DIR}
    ${mfw_core_SOURCE_DIR}
    ${mfw_core_BINARY_DIR}
    ${mfw_wrapper_SOURCE_DIR}
    ${mfw_wrapper_BINARY_DIR}
    ${LPSOLVE_INCLUDE_DIR}
    ${OTB_INCLUDE_DIRS}
    ${RASDAMAN_INCLUDE_DIR}
    ${RASDAMAN_INCLUDE_DIR}/raslib
    ${RASDAMAN_SOURCE_DIR}/applications/rasgeo
    ${PostgreSQL_INCLUDE_DIRS}
    ${lumass_SOURCE_DIR}/utils/ITK
    ${lumass_SOURCE_DIR}
    ${muParserX_SOURCE_DIR}
    ${muparser_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    #${SQLITE3_INCLUDE_DIR}
    ${SQLite_SOURCE_DIR}
    ${NMQSQLite_SRC_DIR}
    ${NMQt_SOURCE_DIR}
    ${SPATIALITE_INCLUDE_DIR}
#    ${TCADB_INCLUDE_DIR}
    ${VisIt_SOURCE_DIR}
    ${VisIt_BINARY_DIR}
    ${NCXX4_INCLUDE_DIR}
    ${NETCDF_INCLUDE_DIR}
    ${NETCDFIO_BINARY_DIR}
    ${pybind11_INCLUDE_DIRS}
    ${bmi_SOURCE_DIR}
    ${bmi_BINARY_DIR}
    ${lumass_SOURCE_DIR}/bmi
    ${lumass_BINARY_DIR}/bmi
    ${YAML_CPP_INCLUDE_DIR}
    ${MPI_CXX_INCLUDE_DIRS}
)

#find_package(Qt5Sql REQUIRED)
#find_package(Qt5Sqlite REQUIRED)

LINK_DIRECTORIES(
        ${QT5_SQLDRIVERS_DIR}
        ${QT5_PREFIX_PATH}/../plugins/sqldrivers
        ${QT5_LINK_DIRS}
        ${OTB_LIBRARY_DIRS}
        ${LIBRARY_OUTPUT_PATH}
        ${NMQt_BINARY_DIR}
        ${mfw_BINARY_DIR}
        ${mfw_BINRAY_DIR}/lib
        ${gui_BINARY_DIR}/lib
        ${SQLite_BINARY_DIR}/lib
        ${muparser_BINARY_DIR}
        ${muParserX_BINARY_DIR}
        ${RASDAMAN_LIBRARIES_DIR}
        ${PostgreSQL_LIBRARY_DIRS}
        ${SPATIALITE_LIB_DIR}
        ${SPATIALITE_LIBLIB_DIR}
        #${QT5SQL_PLUGINS_DIR}/plugins/sqldrivers
        ${OSGEO4W_ROOT}/lib
        ${OSGEO4W_ROOT}/bin
        #${TCADB_LIB_DIR}
        ${VisIt_BINARY_DIR}
        ${NMVtk_BINARY_DIR}
        ${NETCDF_LIB_DIR}
        ${NCXX4_LIB_DIR}
        ${NETCDFIO_BINARY_DIR}
        /opt/ipopt-bin/install/lib
        /opt/mumps-bin/install/lib
        /opt/asl-bin/install/lib
)



QT5_ADD_RESOURCES(NM_RES nmresources.qrc)

SET(CMAKE_AUTOMOC YES)

file(GLOB QT_UI ${gui_SOURCE_DIR}/*.ui)
QT5_WRAP_UI(UI_CPP ${QT_UI})

file(GLOB QT_SRC
    ${gui_SOURCE_DIR}/*.rcc
    ${gui_SOURCE_DIR}/*.cpp
    ${gui_SOURCE_DIR}/*.cxx
    ${gui_SOURCE_DIR}/*.txx
    #${NMQSQLite_SRC_DIR}/*.cpp

    # we also add some other stuff here
    ${shared_SOURCE_DIR}/*.cpp
    #${opt_SOURCE_DIR}/LpHelper.cpp
    #${opt_SOURCE_DIR}/NMMosra.cpp
    ${NMVtk_SOURCE_DIR}/*.cxx
    ${utils_SOURCE_DIR}/ITK/*.cxx
)

# take the ChartsWidget out until we've switched to the new VTK Charts
# framework
list(REMOVE_ITEM QT_SRC ${gui_SOURCE_DIR}/NMChartWidget.cpp)

# take the deprecated vtkOGRLayerMapper out, if applicable
if(DVTKOGL2)
    list(REMOVE_ITEM QT_SRC ${NMVtk_SOURCE_DIR}/vtkOGRLayerMapper.cxx)
    list(REMOVE_ITEM QT_SRC ${NMVtk_SOURCE_DIR}/NMVtkOpenGLPolyDataMapper.cxx)
endif(DVTKOGL2)

# have taken out ${QT_LIBRARIES} here
set(LUMASS_LINKLIBS
    Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Concurrent Qt5::Xml Qt5::Sql
    Qt5::Qml qsqlite
    #Qt5::Network
    Qt5::WebSockets
    QtPropertyBrowser
    NMQt
    ${NCXX4_LIBRARY}
    ${NETCDF_LIBRARY}
    curl xml2
    ${MPI_CXX_LIBRARIES}
    ${HDF5_hdf5_LIBRARY_RELEASE} ${HDF5_hdf5_hl_LIBRARY_RELEASE}
    NMOTBSupplFilters
    NMOTBSupplCore
    NMModFrameCore 
    ${MFW_WRAPPER_NAMES}
    ${LUMASS_GUI_VTK_LIBRARIES}
    ${PostgreSQL_LIBRARIES}  ${LPSOLVE_LIBRARY}
    ${OPENGL_glu_LIBRARY} ${OPENGL_gl_LIBRARY}
    SQLite
    VisItTesselator
    ${YAML_CPP_LIBRARIES}
)

#if (${CMAKE_VERSION} VERSION_LESS_EQUAL "3.11.0")
#    list(APPEND LUMASS_LINKLIBS ${OPENGL_gl_LIBRARY})
#else()
#    list(APPEND LUMASS_LINKLIBS ${OPENGL_opengl_LIBRARY})
#endif()


if(PYTHONSUPPORT)
    add_definitions(-DLUMASS_PYTHON)
    list(APPEND LUMASS_LINK_LIBS ${Python3_LIBRARIES} pybind11::embed)
endif()

set(LUMASS_DEPLIBS
    NMModFrameCore ${MFW_WRAPPER_NAMES}
    QtPropertyBrowser
    SQLite
    VisItTesselator
)

IF(RASSUPPORT)
    add_definitions(-DBUILD_RASSUPPORT)
ENDIF()


# Define a special indicator for Qt Version >= 5.4 (for NMSqliteDriver copy ..)
if (${QT5_VERSION_STRING} VERSION_LESS "5.4.0")
        add_definitions(-DQTVERSION_LT_FIVE_FOUR)
endif()

if (${QT5_VERSION_STRING} VERSION_GREATER "5.5.9")
    add_definitions(-DQT_HIGHDPI_SUPPORT)
endif()

# indicate that we're building the LUMASS GUI VERSION
add_definitions(-DNM_ENABLE_LOGGER)

message(STATUS "LUMASS GUI's linked vtk libraries: ${LUMASS_GUI_VTK_LIBRARIES}")

ADD_EXECUTABLE(lumass ${QT_SRC} ${UI_CPP} ${NM_RES}) #${MOC_CPP}
set_target_properties(lumass PROPERTIES WIN32_EXECUTABLE TRUE)
TARGET_LINK_LIBRARIES(lumass ${LUMASS_LINKLIBS})
ADD_DEPENDENCIES(lumass ${LUMASS_DEPLIBS})

if (${VTK_VERSION_STRING} VERSION_GREATER_EQUAL "8.90")
    vtk_module_autoinit(TARGETS lumass MODULES ${LUMASS_GUI_VTK_LIBRARIES})
endif()

# post-build query of lumass dependencies
#add_custom_command(TARGET lumass POST_BUILD
#                   COMMAND ${CMAKE_SOURCE_DIR}/fetch_ludeps ${lumass_BINARY_DIR}/lib
#                   WORKING_DIRECTORY ${lumass_BINARY_DIR}/bin
#                   VERBATIM
#)

#====================================================================
# INSTALL
#====================================================================

# install rules for lumass executable
install(TARGETS lumass RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

# install for linux AppImage
if (UNIX AND NOT APPLE)

    install(FILES ${lumass_SOURCE_DIR}/gui/resources/LUMASS_icon_64.png
            RENAME lumass.png
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps
    )
    install(FILES ${lumass_SOURCE_DIR}/gui/resources/LUMASS_icon_256.png
            RENAME lumass.png
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps
    )

    install(FILES ${lumass_SOURCE_DIR}/utils/lumass.desktop
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications
    )

    file(GLOB LUMASS_LINUX_UTILS
        ${lumass_SOURCE_DIR}/utils/*.bat
        ${lumass_SOURCE_DIR}/utils/env.sh
        /usr/bin/gdal_rasterize
        /usr/bin/ogr2ogr
        /usr/bin/gdal_polygonize.py
    )

    install(PROGRAMS ${LUMASS_LINUX_UTILS} DESTINATION ${CMAKE_INSTALL_PREFIX}/utils)

endif()
