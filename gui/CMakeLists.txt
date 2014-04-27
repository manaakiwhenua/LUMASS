PROJECT(gui)


INCLUDE_DIRECTORIES(
        ${QT5_INCLUDE_DIRS}
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
	${VTK_INCLUDE_DIRS}
	${OTB_INCLUDE_DIRS}
	${RASDAMAN_INCLUDE_DIR}                                   
	${RASDAMAN_INCLUDE_DIR}/raslib	
	${RASDAMAN_SOURCE_DIR}/applications/rasgeo
	${PostgreSQL_INCLUDE_DIRS}
)     

LINK_DIRECTORIES(
        ${QT5_LINK_DIRS}
	${OTB_LIBRARY_DIRS}
	${VTK_LIBRARY_DIRS}
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
        ${gui_SOURCE_DIR}/NMChartWidget.h
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
)
       
#file(GLOB QT_UI ${gui_SOURCE_DIR}/*.ui)
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
)

# have taken out ${QT_LIBRARIES} here
set(LUMASS_LINKLIBS
        Qt5Widgets Qt5Core Qt5Gui Qt5Concurrent Qt5Xml
        Qt5Declarative Qt5Qml Qt5Quick
        QtPropertyBrowser
	RasImageIO OTBGDALRATImageIO NMModFrame 
	QVTK vtkImaging vtkIO vtkRendering vtkCommon vtkFiltering vtkHybrid vtkInfovis vtkQtChart
	OTBCommon OTBIO OTBBasicFilters 
	${PostgreSQL_LIBRARIES}  ${LPSOLVE_LIBRARY}
	${OPENGL_glu_LIBRARY} 
)

set(LUMASS_DEPLIBS
    NMModFrame RasImageIO OTBGDALRATImageIO
    QtPropertyBrowser 
)

IF(NOT RASSUPPORT)
  list(REMOVE_ITEM LUMASS_LINKLIBS RasImageIO)
  list(REMOVE_ITEM LUMASS_DEPLIBS RasImageIO)
ENDIF()

IF(RASSUPPORT)
    add_definitions(-DBUILD_RASSUPPORT)
ENDIF()

#ADD_DEFINITIONS(
#        ${Qt5Widgets_COMPILE_DEFINITIONS}
#        ${Qt5Xml_COMPILE_DEFINITIONS}
#        ${Qt5Gui_COMPILE_DEFINITIONS}
#        ${Qt5Concurrent_COMPILE_DEFINITIONS}
#        ${Qt5Core_COMPILE_DEFINITIONS}
#        #${Qt5Script_COMPILE_DEFINITIONS}
#        ${Qt5Declarative_COMPILE_DEFINITIONS}
#        ${Qt5Qml_COMPILE_DEFINITIONS}
#        ${Qt5Quick_COMPILE_DEFINITIONS}
#)

#SET(CMAKE_CXX_FLAGS
#        ${CMAKE_CXX_FLAGS}
#        ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}
#        ${Qt5Xml_EXECUTABLE_COMPILE_FLAGS}
#        ${Qt5Gui_EXECUTABLE_COMPILE_FLAGS}
#        ${Qt5Concurrent_EXECUTABLE_COMPILE_FLAGS}
#        ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}
#        #${Qt5Script_EXECUTABLE_COMPILE_FLAGS}
#        ${Qt5Declarative_EXECUTABLE_COMPILE_FLAGS}
#        ${Qt5Qml_EXECUTABLE_COMPILE_FLAGS}
#        ${Qt5Quick_EXECUTABLE_COMPILE_FLAGS}
#)



# have taken out ${UI_CPP}
ADD_EXECUTABLE(lumass ${QT_SRC} ${MOC_CPP}  ${NM_RES})
QT5_USE_MODULES(lumass Widgets Xml Gui Concurrent Core Declarative Qml Quick)
TARGET_LINK_LIBRARIES(lumass ${LUMASS_LINKLIBS})     
ADD_DEPENDENCIES(lumass ${LUMASS_DEPLIBS})

install(TARGETS lumass RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
	
