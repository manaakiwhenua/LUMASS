PROJECT(mfw_wrapper)

INCLUDE_DIRECTORIES(
    ${SPATIALITE_INCLUDE_DIR}
    ${QT5_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
    ${RASDAMAN_SOURCE_DIR}/applications/rasgeo
    ${RASDAMAN_INCLUDE_DIR}
    ${RASDAMAN_INCLUDE_DIR}/raslib
    ${VTK_INCLUDE_DIRS}
    ${mfw_core_BINARY_DIR}
    ${mfw_core_SOURCE_DIR}
    ${mfw_wrapper_SOURCE_DIR}
    ${mfw_wrapper_BINARY_DIR}
    ${muParserX_SOURCE_DIR}
    ${shared_SOURCE_DIR}
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${rimageio_SOURCE_DIR}
    ${filters_SOURCE_DIR}
    ${filters_BINARY_DIR}
    ${gui_SOURCE_DIR}
    ${lumass_SOURCE_DIR}/utils/ITK
    ${lumass_SOURCE_DIR}
    ${otbsuppl_core_SOURCE_DIR}
    ${OTB_INCLUDE_DIRS}
    ${LPSOLVE_INCLUDE_DIR}
    ${opt_SOURCE_DIR}
)

LINK_DIRECTORIES(
    ${OTB_DIR}/lib
    ${OTB_DIR}/bin
    ${QT5_LINK_DIRS}
    ${RASDAMAN_LIBRARIES_DIR}
    ${mfw_core_BINARY_DIR}
    ${LIBRARY_OUTPUT_PATH}
    ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}
    ${OSGEO4W_ROOT}/lib
    ${SPATIALITE_LIB_DIR}
    ${SPATIALITE_LIBLIB_DIR}
)

set(MFW_WRAPPER_LINK_LIBS
                NMModFrameCore
                OTBSupplFilters
                #RasImageIO
                #Qt5Xml
                Qt5Core
                #Qt5Qml
                #Qt5Concurrent
                #spatialite
                vtkCommonDataModel
                vtkCommonCore
                vtkIOCore
                ${LPSOLVE_LIBRARY}
)


set(MFW_WRAPPER_DEP_LIBS
    OTBSupplFilters
    NMModFrameCore
)


if (NOT RASSUPPORT)
    list(REMOVE_ITEM MFW_WRAPPER_NAMES "NMRasdamanConnector")
endif()


IF(RASSUPPORT)
    add_definitions(-DBUILD_RASSUPPORT)
    LIST(APPEND MFW_WRAPPER_LINK_LIBS RasImageIO)
    #LIST(APPEND MFW_DEP_LIBS RasImageIO)
ENDIF()

add_definitions(-DNMModFrameWrapper_EXPORTS -DNM_PROC_LOG)
if (LUMASS_OTB_VERSION VERSION_GREATER "5.8.0")
    add_definitions(-DOTB_VERSION_SIX)
endif()

set(CMAKE_AUTOMOC YES)

foreach( wrapper ${MFW_WRAPPER_NAMES} )
        file(GLOB MFW_WRAPPER_SOURCE_FILES "${mfw_wrapper_SOURCE_DIR}/${wrapper}*.c*")
        file(GLOB MFW_WRAPPER_HEADER_FILES "${mfw_wrapper_SOURCE_DIR}/${wrapper}*.h")

        if (${wrapper} STREQUAL "NMMosraFilterWrapper")
            list(APPEND MFW_WRAPPER_SOURCE_FILES ${opt_SOURCE_DIR}/NMMosra.cpp)
            list(APPEND MFW_WRAPPER_SOURCE_FILES ${opt_SOURCE_DIR}/LpHelper.cpp)
            list(APPEND MFW_WRAPPER_HEADER_FILES ${opt_SOURCE_DIR}/NMMosra.h)
            list(APPEND MFW_WRAPPER_HEADER_FILES ${opt_SOURCE_DIR}/LpHelper.h)
            #list(APPEND MFW_WRAPPER_SOURCE_FILES ${gui_SOURCE_DIR}/NMSqlTableModel.cpp)
            #list(APPEND MFW_WRAPPER_HEADER_FILES ${gui_SOURCE_DIR}/NMSqlTableModel.h)
            list(APPEND MFW_WRAPPER_LINK_LIBS Qt5Sql)
        elseif (${wrapper} STREQUAL "NMJSKernelFilterWrapper")
            list(APPEND MFW_WRAPPER_LINK_LIBS Qt5Qml)
        endif()

        #message(STATUS "source files in lib ${MFW_WRAPPER_SOURCE_FILES}")
        #message(STATUS "source files in lib ${MFW_WRAPPER_HEADER_FILES}")

        include(GenerateExportHeader)
        ADD_LIBRARY(${wrapper} SHARED ${MFW_WRAPPER_SOURCE_FILES})

        generate_export_header(${wrapper})

        TARGET_LINK_LIBRARIES(${wrapper} ${MFW_WRAPPER_LINK_LIBS})
        ADD_DEPENDENCIES(${wrapper} ${MFW_WRAPPER_DEP_LIBS})

        install(FILES ${MFW_WRAPPER_HEADER_FILES} DESTINATION include)
        if(WIN32)
                install(TARGETS ${wrapper}
                        LIBRARY DESTINATION lib
                        RUNTIME DESTINATION lib
                )
        else()
                install(TARGETS ${wrapper} LIBRARY DESTINATION lib)
        endif()

endforeach()
