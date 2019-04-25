PROJECT(mfw_wrapper)


#find_package(VTK COMPONENTS
#        vtkCommonDataModel
#        vtkCommonCore
#        vtkIOCore
#)

#include(${VTK_USE_FILE})

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

#file(GLOB MFW_WRAPPER_CPP
#   ${mfw_wrapper_SOURCE_DIR}/*.cpp
#   ${mfw_wrapper_SOURCE_DIR}/*.cxx
#   ${lumass_SOURCE_DIR}/util/ITK/itkVTKImageExportBase.cxx
##   ${shared_SOURCE_DIR}/NMLogger.cpp
##   ${shared_SOURCE_DIR}/NMObject.cpp
#   ${opt_SOURCE_DIR}/LpHelper.cpp
#   ${opt_SOURCE_DIR}/NMMosra.cpp
#)

set(MFW_WRAPPER_LINK_LIBS
                #OTBGDALRATImageIO
                #OTBMathParser
                NMModFrameCore
                OTBSupplFilters
                #RasImageIO
                #Qt5Xml
                Qt5Core
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

#SET(MFW_WRAPPER_NAMES
#    NMMosraFilterWrapper
#    #NMScriptableKernelFilter2Wrapper
#)


if (NOT RASSUPPORT)
    list(REMOVE_ITEM MFW_WRAPPER_NAMES "NMRasdamanConnector")
endif()


#file(GLOB MFW_WRAPPER_H
#   ${mfw_wrapper_SOURCE_DIR}/*.h
##   ${shared_SOURCE_DIR}/NMLogger.h
##   ${shared_SOURCE_DIR}/NMObject.h
#   ${opt_SOURCE_DIR}/LpHelper.h
#   ${opt_SOURCE_DIR}/NMMosra.h
#)

# those two classes are part of the NMModFrameCore library
#LIST(REMOVE_ITEM MFW_WRAPPER_CPP ${mfw_wrapper_SOURCE_DIR}/NMImageReader.cxx)
#LIST(REMOVE_ITEM MFW_WRAPPER_H ${mfw_wrapper_SOURCE_DIR}/NMImageReader.h)
#LIST(REMOVE_ITEM MFW_WRAPPER_CPP ${mfw_wrapper_SOURCE_DIR}/NMTableReader.cpp)
#LIST(REMOVE_ITEM MFW_WRAPPER_H ${mfw_wrapper_SOURCE_DIR}/NMTableReader.h)
#LIST(REMOVE_ITEM MFW_WRAPPER_CPP ${mfw_wrapper_SOURCE_DIR}/NMItkDataObjectWrapper.cpp)
#LIST(REMOVE_ITEM MFW_WRAPPER_H ${mfw_wrapper_SOURCE_DIR}/NMItkDataObjectWrapper.h)

## ... take even more out we're not really using
#LIST(REMOVE_ITEM MFW_WRAPPER_CPP ${mfw_wrapper_SOURCE_DIR}/NMCombineTwoFilterWrapper.cpp)
#LIST(REMOVE_ITEM MFW_WRAPPER_H ${mfw_wrapper_SOURCE_DIR}/NMCombineTwoFilterWrapper.h)
#LIST(REMOVE_ITEM MFW_WRAPPER_CPP ${mfw_wrapper_SOURCE_DIR}/NMExternalSortFilterWrapper.cpp)
#LIST(REMOVE_ITEM MFW_WRAPPER_H ${mfw_wrapper_SOURCE_DIR}/NMExternalSortFilterWrapper.h)
#LIST(REMOVE_ITEM MFW_WRAPPER_CPP ${mfw_wrapper_SOURCE_DIR}/NMLUAllocationWrapper.cpp)
#LIST(REMOVE_ITEM MFW_WRAPPER_H ${mfw_wrapper_SOURCE_DIR}/NMLUAllocationWrapper.h)
#LIST(REMOVE_ITEM MFW_WRAPPER_CPP ${mfw_wrapper_SOURCE_DIR}/NMNeighbourhoodCountingWrapper.cpp)
#LIST(REMOVE_ITEM MFW_WRAPPER_H ${mfw_wrapper_SOURCE_DIR}/NMNeighbourhoodCountingWrapper.h)
#LIST(REMOVE_ITEM MFW_WRAPPER_CPP ${mfw_wrapper_SOURCE_DIR}/NMSQLProcessor.cpp)
#LIST(REMOVE_ITEM MFW_WRAPPER_H ${mfw_wrapper_SOURCE_DIR}/NMSQLProcessor.h)
#LIST(REMOVE_ITEM MFW_WRAPPER_CPP ${mfw_wrapper_SOURCE_DIR}/NMOtbAttributeTableWrapper.cpp)
#LIST(REMOVE_ITEM MFW_WRAPPER_H ${mfw_wrapper_SOURCE_DIR}/NMOtbAttributeTableWrapper.h)
#LIST(REMOVE_ITEM MFW_WRAPPER_CPP ${mfw_wrapper_SOURCE_DIR}/NMScriptableKernelFilterWrapper.cpp)
#LIST(REMOVE_ITEM MFW_WRAPPER_H ${mfw_wrapper_SOURCE_DIR}/NMScriptableKernelFilterWrapper.h)
#LIST(REMOVE_ITEM MFW_WRAPPER_CPP ${mfw_wrapper_SOURCE_DIR}/NMFocalNeighbourhoodDistanceWeightingWrapper.cpp)
#LIST(REMOVE_ITEM MFW_WRAPPER_H ${mfw_wrapper_SOURCE_DIR}/NMFocalNeighbourhoodDistanceWeightingWrapper.h)
#LIST(REMOVE_ITEM MFW_WRAPPER_CPP ${mfw_wrapper_SOURCE_DIR}/NMVirtualStreamWriter.cpp)
#LIST(REMOVE_ITEM MFW_WRAPPER_H ${mfw_wrapper_SOURCE_DIR}/NMVirtualStreamWriter.h)


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
        endif()

#        if (${wrapper} STREQUAL "NMCostDistanceBufferImageWrapper" OR ${wrapper} STREQUAL "UniqueCombinationFilterWrapper")

#        endif()


        message(STATUS "source files in lib ${MFW_WRAPPER_SOURCE_FILES}")
        message(STATUS "source files in lib ${MFW_WRAPPER_HEADER_FILES}")


        #QT5_WRAP_CPP(MFW__MOC_CPP ${MFW_WRAPPER_HEADER_FILES})
        #message(STATUS "moc files: ${MFW_WRAPPER_MOC_CPP}")

        include(GenerateExportHeader)
        ADD_LIBRARY(${wrapper} SHARED ${MFW_WRAPPER_SOURCE_FILES}) #${MFW_WRAPPER_MOC_CPP})

        generate_export_header(${wrapper})

        #QT5_USE_MODULES(NMModFrame Core Xml Concurrent)
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
