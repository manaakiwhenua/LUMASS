PROJECT(gui)

if (${VTK_VERSION_STRING} VERSION_LESS "6.3")
find_package(VTK COMPONENTS
	vtkCommonDataModel
	vtkCommonCore
	vtkFiltersGeneric
	vtkRenderingCore
	vtkRenderingAnnotation
	vtkRenderingImage
	vtkRenderingOpenGL
	vtkGUISupportQt
	vtkImagingStatistics
	vtkInteractionWidgets
	vtkInteractionStyle
    vtkRenderingFreeTypeOpenGL
    #vtkRenderingFreeType
	vtkIOCore
	vtkIOInfovis
	vtkIOSQL
	vtkIOLegacy
	vtkIOXML
	NO_MODULE
)
else()
find_package(VTK COMPONENTS
	vtkCommonDataModel
	vtkCommonCore
	vtkFiltersGeneric
	vtkRenderingCore
	vtkRenderingAnnotation
	vtkRenderingImage
	vtkRenderingOpenGL
	vtkGUISupportQt
	vtkImagingStatistics
	vtkInteractionWidgets
	vtkInteractionStyle
	#vtkRenderingFreeTypeOpenGL
	vtkRenderingFreeType
	vtkIOCore
	vtkIOInfovis
	vtkIOSQL
	vtkIOLegacy
	vtkIOXML
	NO_MODULE
)
endif()

include(${VTK_USE_FILE})


INCLUDE_DIRECTORIES(
    ${QT5_INCLUDE_DIRS}
    ${QT5CORE_PRIVATE_DIR}
    ${QT5SQL_INCLUDE_DIR}
    #${QT5SQL_PRIVATE_DIR}/private
    ${opt_SOURCE_DIR}
    ${lumass_BINARY_DIR}
    ${gui_BINARY_DIR}
    ${utils_SOURCE_DIR}/QtPropertyBrowser/src
    ${NMQSQLite_SRC_DIR}
    ${NMVtk_SOURCE_DIR}
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${gui_SOURCE_DIR}
    ${filters_SOURCE_DIR}
    ${otbsuppl_core_SOURCE_DIR}
    ${filters_BINARY_DIR}
    ${shared_SOURCE_DIR}
    ${rimageio_SOURCE_DIR}
    ${mfw_SOURCE_DIR}
    ${mfw_BINARY_DIR}
    ${mfw_wrapper_SOURCE_DIR}
    ${LPSOLVE_INCLUDE_DIR}
    ${OTB_INCLUDE_DIRS}
    ${RASDAMAN_INCLUDE_DIR}
    ${RASDAMAN_INCLUDE_DIR}/raslib
    ${RASDAMAN_SOURCE_DIR}/applications/rasgeo
    ${PostgreSQL_INCLUDE_DIRS}
    ${lumass_SOURCE_DIR}/utils/ITK
	${lumass_SOURCE_DIR}
    ${muParserX_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    #${SQLITE3_INCLUDE_DIR}
    ${SQLite_SOURCE_DIR}
    ${NMQSQLite_SRC_DIR}
	${SPATIALITE_INCLUDE_DIR}
#    ${TCADB_INCLUDE_DIR}
)     

LINK_DIRECTORIES(
    ${QT5_LINK_DIRS}
	${OTB_LIBRARY_DIRS}
	${LIBRARY_OUTPUT_PATH}
	${mfw_BINARY_DIR}
	${mfw_BINRAY_DIR}/lib
	${gui_BINARY_DIR}/lib
	${SQLite_BINARY_DIR}/lib
	${muparser_BINARY_DIR}  
    ${muParserX_BINARY_DIR}
	${RASDAMAN_LIBRARIES_DIR}
	${PostgreSQL_LIBRARY_DIRS}
	${SPATIALITE_LIB_DIR}
	${SPATIALTE_LIBLIB_DIR}
    #${QT5SQL_PLUGINS_DIR}/plugins/sqldrivers
	${OSGEO4W_ROOT}/lib
	${OSGEO4W_ROOT}/bin
	#${TCADB_LIB_DIR}
)



QT5_ADD_RESOURCES(NM_RES nmresources.qrc)

QT5_WRAP_CPP(MOC_CPP
        ${NMVtk_SOURCE_DIR}/vtkQtEditableTableModelAdapter.h
        ${opt_SOURCE_DIR}/NMMosra.h
        ${gui_SOURCE_DIR}/otbmodellerwin.h
        ${gui_SOURCE_DIR}/modelcomponentlist.h
        ${gui_SOURCE_DIR}/NMLayer.h
        ${gui_SOURCE_DIR}/NMVectorLayer.h
        ${gui_SOURCE_DIR}/NMImageLayer.h
        ${gui_SOURCE_DIR}/NMLayerModel.h
        ${gui_SOURCE_DIR}/NMQtOtbAttributeTableModel.h
        ${gui_SOURCE_DIR}/NMTableView.h
        ${gui_SOURCE_DIR}/NMSqlTableView.h
        ${gui_SOURCE_DIR}/NMSqlTableModel.h
#        ${gui_SOURCE_DIR}/NMChartWidget.h
        ${gui_SOURCE_DIR}/NMAddColumnDialog.h
        ${gui_SOURCE_DIR}/NMTableCalculator.h
        ${gui_SOURCE_DIR}/NMSelectableSortFilterProxyModel.h
        ${gui_SOURCE_DIR}/NMSelSortSqlTableProxyModel.h
        ${gui_SOURCE_DIR}/NMItk2VtkConnector.h
        ${gui_SOURCE_DIR}/NMProcCompList.h
        ${gui_SOURCE_DIR}/NMEditModelComponentDialog.h
        ${gui_SOURCE_DIR}/NMModelViewWidget.h
        ${gui_SOURCE_DIR}/NMModelScene.h
        ${gui_SOURCE_DIR}/NMProcessComponentItem.h
        ${gui_SOURCE_DIR}/NMAggregateComponentItem.h
        ${gui_SOURCE_DIR}/NMComponentLinkItem.h
        ${gui_SOURCE_DIR}/NMFastTrackSelectionModel.h
        ${gui_SOURCE_DIR}/NMComponentListItemDelegate.h
        ${gui_SOURCE_DIR}/NMComponentEditor.h
        ${gui_SOURCE_DIR}/NMWidgetListView.h
        ${NMQSQLite_SRC_DIR}/nmqsqlcachedresult_p.h
        ${NMQSQLite_SRC_DIR}/nmqsql_sqlite_p.h
)

file(GLOB QT_UI ${gui_SOURCE_DIR}/*.ui)
QT5_WRAP_UI(UI_CPP ${QT_UI})
       
file(GLOB QT_SRC 
    ${gui_SOURCE_DIR}/*.rcc 
    ${gui_SOURCE_DIR}/*.cpp 
    ${gui_SOURCE_DIR}/*.cxx
    ${gui_SOURCE_DIR}/*.txx 
    ${NMQSQLite_SRC_DIR}/*.cpp
    
    # we also add some other stuff here
    # want to be included
    ${opt_SOURCE_DIR}/*.cpp
    ${NMVtk_SOURCE_DIR}/*.cxx
    ${lumass_SOURCE_DIR}/utils/ITK/itkVTKImageExportBase.cxx
)

# take the ChartsWidget out until we've switched to the new VTK Charts
# framework
list(REMOVE_ITEM QT_SRC ${gui_SOURCE_DIR}/NMChartWidget.cpp)

# have taken out ${QT_LIBRARIES} here
set(LUMASS_LINKLIBS
    Qt5Widgets Qt5Core Qt5Gui Qt5Concurrent Qt5Xml Qt5Sql
    #qsqlite
    #Qt5Declarative Qt5Qml Qt5Quick
	QtPropertyBrowser
	NMModFrame
	#QVTK vtkImaging vtkIO vtkRendering vtkCommon vtkFiltering vtkHybrid vtkInfovis vtkQtChart
	${VTK_LIBRARIES}
	${PostgreSQL_LIBRARIES}  ${LPSOLVE_LIBRARY}
	${OPENGL_glu_LIBRARY}
	#${SQLITE3_LIBRARY}
	SQLite 
	#SQLiteExtFunc
)

# need to add the import library on windows
#if(WIN32)
#	LIST(REMOVE_ITEM LUMASS_LINKLIBS spatialite)
#	LIST(APPEND LUMASS_LINKLIBS spatialite_i)
#endif()

set(LUMASS_DEPLIBS
	NMModFrame
	QtPropertyBrowser 
	SQLite
	#SQLiteExtFunc
)


IF(RASSUPPORT)
    add_definitions(-DBUILD_RASSUPPORT)
ENDIF()

# Define a special indicator for Qt Version >= 5.4 (for NMSqliteDriver copy ..)
if (${QT5_VERSION_STRING} VERSION_LESS "5.4.0")
	add_definitions(-DQTVERSION_LT_FIVE_FOUR)
endif()

# Qt5 requirement
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -fPIC")

message(STATUS "vtk definitions: ${VTK_DEFINITIONS}")

# VTK6 requirement
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS "${VTK_DEFINITIONS} ")

ADD_EXECUTABLE(lumass ${QT_SRC} ${MOC_CPP} ${UI_CPP} ${NM_RES})
#QT5_USE_MODULES(lumass Widgets Xml Gui Concurrent Core Declarative Qml Quick)
TARGET_LINK_LIBRARIES(lumass ${LUMASS_LINKLIBS})     
ADD_DEPENDENCIES(lumass ${LUMASS_DEPLIBS})

# post-build query of lumass dependencies
#add_custom_command(TARGET lumass POST_BUILD
#                   COMMAND ${CMAKE_SOURCE_DIR}/fetch_ludeps ${lumass_BINARY_DIR}/lib
#                   WORKING_DIRECTORY ${lumass_BINARY_DIR}/bin
#                   VERBATIM
#)

# install rules for lumass executable
install(TARGETS lumass RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
	
