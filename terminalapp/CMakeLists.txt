PROJECT(lumassengine)

INCLUDE_DIRECTORIES(
	${opt_SOURCE_DIR}
	${gui_SOURCE_DIR}
	${shared_SOURCE_DIR}
	${lumass_BINARY_DIR}
 	${GDALRATImageIO_SOURCE_DIR}
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
	${OTB_LIBRARY_DIRS}
	${VTK_LIBRARY_DIRS}
	${LIBRARY_OUTPUT_PATH}
	${mfw_BINARY_DIR}
	${mfw_BINRAY_DIR}/lib
	${muparser_BINARY_DIR}  
	${RASDAMAN_LIBRARIES_DIR}
	${PostgreSQL_LIBRARY_DIRS}
)

QT4_WRAP_CPP(ENGINE_MOC_CPP 
	${opt_SOURCE_DIR}/NMMosra.h
	${opt_SOURCE_DIR}/MOSORunnable.h
	#${gui_SOURCE_DIR}/NMTableCalculator.h	
	#${gui_SOURCE_DIR}/NMItk2VtkConnector.h
)

file(GLOB ENGINE_SRC 
    ${opt_SOURCE_DIR}/*.cpp 
    ${lumassengine_SOURCE_DIR}/*.cxx 
    #${gui_SOURCE_DIR}/NMTableCalculator.cpp 
    #${gui_SOURCE_DIR}/NMItk2VtkConnector.cpp
)

set(ENGINE_LINKLIBS QtCore QtXml 
	RasImageIO OTBGDALRATImageIO NMModFrame 
	vtkImaging vtkIO vtkCommon vtkFiltering vtkHybrid 
	OTBCommon OTBIO OTBBasicFilters 
	${PostgreSQL_LIBRARIES}  ${LPSOLVE_LIBRARY}
)

set(ENGINE_DEPLIBS
    NMModFrame RasImageIO OTBGDALRATImageIO
)

IF(NOT RASSUPPORT)
  IF (ENGINE_LINKLIBS)
  	list(REMOVE_ITEM ENGINE_LINKLIBS RasImageIO)
  ENDIF()
  IF (ENGINE_DEPLIBS)
  	list(REMOVE_ITEM ENGINE_DEPLIBS RasImageIO)
  ENDIF()
ENDIF()

IF(RASSUPPORT)
    add_definitions(-DBUILD_RASSUPPORT)
ENDIF()

ADD_EXECUTABLE(lumassengine ${ENGINE_SRC} ${ENGINE_MOC_CPP})
TARGET_LINK_LIBRARIES(lumassengine ${ENGINE_LINKLIBS})
ADD_DEPENDENCIES(lumassengine ${ENGINE_DEPLIBS})

install(TARGETS lumassengine DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
