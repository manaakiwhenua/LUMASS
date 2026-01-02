#******************************************************************************
# Created by Alexander Herzig
# Copyright 2025-2026 New Zealand Institute for Bioeconomy Science Limited
# 
# This file is part of 'LUMASS', which is free software: you can redistribute
# it and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 3 of the License,
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#*******************************************************************************

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
    #${muParserX_SOURCE_DIR}
    ${shared_SOURCE_DIR}
    ${shared_BINARY_DIR}
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${NETCDFIO_SOURCE_DIR}
    ${NETCDFIO_BINARY_DIR}
    ${rimageio_SOURCE_DIR}
    ${filters_SOURCE_DIR}
    ${filters_BINARY_DIR}
    ${gui_SOURCE_DIR}
    ${lumass_SOURCE_DIR}/utils/ITK
    ${lumass_SOURCE_DIR}
    ${OTBSupplCore_SOURCE_DIR}
    ${OTBSupplCore_BINARY_DIR}
    ${OTB_INCLUDE_DIRS}
    ${LPSOLVE_INCLUDE_DIR}
    ${opt_SOURCE_DIR}
    ${NMVtk_SOURCE_DIR}
    ${NCXX4_INCLUDE_DIR}
    ${NETCDF_INCLUDE_DIR}
    ${pybind11_INCLUDE_DIRS}
    ${lumass_SOURCE_DIR}/bmi
    ${lumass_BINARY_DIR}/bmi
    ${bmi_SOURCE_DIR}
    ${bmi_BINARY_DIR}
    ${YAML_CPP_INCLUDE_DIR}
    ${MPI_CXX_INCLUDE_DIRS}
)

LINK_DIRECTORIES(
        #${OTB_DIR}/lib
        #${OTB_DIR}/bin
        #${QT5_LINK_DIRS}
    ${NETCDF_LIB_DIR}
    ${NCXX4_LIB_DIR}
        #${RASDAMAN_LIBRARIES_DIR}
        #${mfw_core_BINARY_DIR}
        #${LIBRARY_OUTPUT_PATH}
        #${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}
    ${OSGEO4W_ROOT}/lib
    ${NMQt_SOURCE_DIR}
        #${SPATIALITE_LIB_DIR}
        #${SPATIALITE_LIBLIB_DIR}
        #${bmi_BINARY_DIR}
    ${YAML_CPP_LIB_DIR}
    ${OTBSupplCore_BINARY_DIR}
    ${LIBRARY_OUTPUT_PATH}
)


if (${VTK_VERSION_STRING} VERSION_LESS "8.90")
    SET(LUMASS_WRAPPER_VTK_LIBRARIES
        vtkCommonDataModel
        vtkCommonCore
        vtkIOCore
    )
else()
   SET(LUMASS_WRAPPER_VTK_LIBRARIES
       VTK::CommonDataModel
       VTK::CommonCore
       VTK::IOCore
   )
endif()


set(MFW_WRAPPER_LINK_LIBS
    NMOTBSupplCoreRAT 
    NMOTBSupplCoreIO
    NMOTBSupplCoreReader
    NMOTBSupplCoreWriter
    NMModFrameCore
    NMOTBSupplFilters
    Qt5::Core
        #${QT5_LIBRARIES}
    ${LUMASS_WRAPPER_VTK_LIBRARIES}
    OTBTestKernel
)

set(NMMOSRA_WRAPPER_LINKLIBS ${MFW_WRAPPER_LINK_LIBS} Qt5::Sql NMQt NMMosra ${LPSOLVE_LIBRARY})
set(NMJSKERNEL_WRAPPER_LINKLIBS ${MFW_WRAPPER_LINK_LIBS} Qt5::Qml)
set(NMBMI_WRAPPER_LINKLIBS ${MFW_WRAPPER_LINK_LIBS} ${YAML_CPP_LIBRARIES})

set(MFW_WRAPPER_DEP_LIBS
    NMOTBSupplCoreRAT 
    NMOTBSupplCoreIO
    NMOTBSupplCoreReader
    NMOTBSupplCoreWriter
    NMOTBSupplFilters
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

if (MSVC)
    add_compile_options(/bigobj)
endif()


set(CMAKE_AUTOMOC YES)

string(TOLOWER ${CMAKE_BUILD_TYPE} BLDTYPE)
string(COMPARE EQUAL ${BLDTYPE} "debug" HAVEDEBUG)

#if (HAVEDEBUG AND NOT WIN32)
#    LIST(APPEND MFW_WRAPPER_NAMES "NMUniqueCombinationFilterWrapper")
#endif()

foreach( wrapper ${MFW_WRAPPER_NAMES} )
        file(GLOB MFW_WRAPPER_SOURCE_FILES "${mfw_wrapper_SOURCE_DIR}/${wrapper}*.c*")
        file(GLOB MFW_WRAPPER_HEADER_FILES "${mfw_wrapper_SOURCE_DIR}/${wrapper}*.h")

        set(LINKLIBS ${MFW_WRAPPER_LINK_LIBS})
        if (${wrapper} STREQUAL "NMMosraFilterWrapper")
            #list(APPEND MFW_WRAPPER_SOURCE_FILES ${opt_SOURCE_DIR}/NMMosra.cpp)
            #list(APPEND MFW_WRAPPER_SOURCE_FILES ${opt_SOURCE_DIR}/LpHelper.cpp)
            #list(APPEND MFW_WRAPPER_HEADER_FILES ${opt_SOURCE_DIR}/NMMosra.h)
            #list(APPEND MFW_WRAPPER_HEADER_FILES ${opt_SOURCE_DIR}/LpHelper.h)
            #list(APPEND MFW_WRAPPER_SOURCE_FILES ${NMVtk_SOURCE_DIR}/NMvtkDelimitedTextWriter.cxx)
            #list(APPEND MFW_WRAPPER_HEADER_FILES ${NMVtk_SOURCE_DIR}/NMvtkDelimitedTextWriter.h)
            set(LINKLIBS ${NMMOSRA_WRAPPER_LINKLIBS})
        elseif (${wrapper} STREQUAL "NMJSKernelFilterWrapper")
            set(LINKLIBS ${NMJSKERNEL_WRAPPER_LINKLIBS})
        elseif (${wrapper} STREQUAL "NMBMIWrapper")
            set(LINKLIBS ${NMBMI_WRAPPER_LINKLIBS})
            if (PYTHONSUPPORT)
                add_definitions(-DLUMASS_PYTHON)
                list(APPEND LINKLIBS
                        ${Python3_LIBRARIES}
                        pybind11::embed
                        pybind11::pybind11
                        LumassPythonBMI
                     )
            endif()
        endif()

        message(STATUS "LUMASS component ${wrapper} enabled!")
        #message(STATUS "${wrapper}'s LINK_LIBS: ${LINKLIBS}")


        include(GenerateExportHeader)
        ADD_LIBRARY(${wrapper} SHARED ${MFW_WRAPPER_SOURCE_FILES})

        generate_export_header(${wrapper})

        TARGET_LINK_LIBRARIES(${wrapper} PUBLIC ${LINKLIBS})
        ADD_DEPENDENCIES(${wrapper} ${MFW_WRAPPER_DEP_LIBS})

        if (${VTK_VERSION_STRING} VERSION_GREATER_EQUAL "8.90")
            vtk_module_autoinit(TARGETS ${wrapper} MODULES ${LUMASS_WRAPPER_VTK_LIBRARIES})
        endif()

        #install(FILES ${MFW_WRAPPER_HEADER_FILES} DESTINATION include)
        if(WIN32)
                install(TARGETS ${wrapper}
                        LIBRARY DESTINATION lib
                        RUNTIME DESTINATION bin
                )
        else()
                install(TARGETS ${wrapper} LIBRARY DESTINATION lib)
        endif()

endforeach()


# add 'manual' entry for NMUniqueCombinationFilterWrapper release builds, as
# it causes some grief ...
#if (NOT HAVEDEBUG AND NOT WIN32)
    set(wrapper "NMUniqueCombinationFilterWrapper")
    file(GLOB MFW_WRAPPER_SOURCE_FILES "${mfw_wrapper_SOURCE_DIR}/${wrapper}*.c*")
    file(GLOB MFW_WRAPPER_HEADER_FILES "${mfw_wrapper_SOURCE_DIR}/${wrapper}*.h")

    set(LINKLIBS ${MFW_WRAPPER_LINK_LIBS})
    message(STATUS "LUMASS component ${wrapper} enabled!")
    #message(STATUS "${wrapper}'s LINK_LIBS: ${LINKLIBS}")

    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O0 -g -fPIC -fno-aggressive-loop-optimizations")

    include(GenerateExportHeader)
    ADD_LIBRARY(${wrapper} SHARED ${MFW_WRAPPER_SOURCE_FILES})

    generate_export_header(${wrapper})

    TARGET_LINK_LIBRARIES(${wrapper} PUBLIC ${LINKLIBS})
    ADD_DEPENDENCIES(${wrapper} ${MFW_WRAPPER_DEP_LIBS})

    if(WIN32)
            install(TARGETS ${wrapper}
                    LIBRARY DESTINATION lib
                    RUNTIME DESTINATION bin
            )
    else()
            install(TARGETS ${wrapper} LIBRARY DESTINATION lib)
    endif()
#endif()


#if(UNIX AND NOT APPLE)
#
#    file(GLOB LUMASS_LINUX_ALLLIBS
#        ${lumass_BINARY_DIR}/lib/*so*
#    )
#
#    install(FILES ${LUMASS_LINUX_ALLLIBS} DESTINATION lib)
#
#endif()

