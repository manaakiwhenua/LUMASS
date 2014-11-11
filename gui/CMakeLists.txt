PROJECT(gui)

find_package(VTK COMPONENTS
vtkCommonCore
vtkCommonDataModel
vtkFiltersGeneric
vtkRenderingCore
vtkRenderingAnnotation
vtkRenderingImage
vtkGUISupportQt
vtkImagingStatistics
vtkInteractionWidgets
vtkIOCore
vtkIOInfovis
vtkIOSQL
vtkIOLegacy
vtkIOXML
)

include(${VTK_USE_FILE})


INCLUDE_DIRECTORIES(
        ${QT5_INCLUDE_DIRS}
        ${QT5CORE_PRIVATE_DIR}
        ${opt_SOURCE_DIR}
	${lumass_BINARY_DIR}
        ${gui_BINARY_DIR}
	${utils_SOURCE_DIR}/QtPropertyBrowser/src
	${NMVtk_SOURCE_DIR}
	${GDALRATImageIO_SOURCE_DIR}
	${gui_SOURCE_DIR}
	${filters_SOURCE_DIR}
	${shared_SOURCE_DIR}
	${rimageio_SOURCE_DIR}
	${mfw_SOURCE_DIR}    
	${mfw_wrapper_SOURCE_DIR}
        ${LPSOLVE_INCLUDE_DIR}
        #${VTK_INCLUDE_DIRS}
	${OTB_INCLUDE_DIRS}
	${RASDAMAN_INCLUDE_DIR}                                   
	${RASDAMAN_INCLUDE_DIR}/raslib	
	${RASDAMAN_SOURCE_DIR}/applications/rasgeo
	${PostgreSQL_INCLUDE_DIRS}
        ${lumass_SOURCE_DIR}/utils/ITK
)     

LINK_DIRECTORIES(
        ${QT5_LINK_DIRS}
	${OTB_LIBRARY_DIRS}
        #${VTK_LIBRARY_DIRS}
	${LIBRARY_OUTPUT_PATH}
	${mfw_BINARY_DIR}
	${mfw_BINRAY_DIR}/lib
	${gui_BINARY_DIR}/lib
	${muparser_BINARY_DIR}  
	${RASDAMAN_LIBRARIES_DIR}
	${PostgreSQL_LIBRARY_DIRS}
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
        #${gui_SOURCE_DIR}/NMChartWidget.h
        ${gui_SOURCE_DIR}/NMAddColumnDialog.h
        ${gui_SOURCE_DIR}/NMTableCalculator.h
        ${gui_SOURCE_DIR}/NMSelectableSortFilterProxyModel.h
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
)

file(GLOB QT_UI ${gui_SOURCE_DIR}/*.ui)
QT5_WRAP_UI(UI_CPP ${QT_UI})
       
file(GLOB QT_SRC 
    ${gui_SOURCE_DIR}/*.rcc 
    ${gui_SOURCE_DIR}/*.cpp 
    ${gui_SOURCE_DIR}/*.cxx
    ${gui_SOURCE_DIR}/*.txx 
    
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
        Qt5Widgets Qt5Core Qt5Gui Qt5Concurrent Qt5Xml
        #Qt5Declarative Qt5Qml Qt5Quick
        QtPropertyBrowser
        NMModFrame
        #QVTK vtkImaging vtkIO vtkRendering vtkCommon vtkFiltering vtkHybrid vtkInfovis vtkQtChart
        ${VTK_LIBRARIES}
        ${PostgreSQL_LIBRARIES}  ${LPSOLVE_LIBRARY}
	${OPENGL_glu_LIBRARY} 
)

set(LUMASS_DEPLIBS
    NMModFrame
    QtPropertyBrowser 
)

IF(RASSUPPORT)
    add_definitions(-DBUILD_RASSUPPORT)
ENDIF()

# Qt5 requirement
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE")

# VTK6 requirement
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS ${VTK_DEFINITIONS})

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
	
