PROJECT(VisIt)

if (${VTK_VERSION_STRING} VERSION_LESS "8.90")
    find_package(VTK COMPONENTS
            vtkCommonDataModel
            vtkCommonCore
            vtkIOCore
    )
    set(VTK_LIBRARIES
        vtkCommonDataModel
        vtkCommonCore
        vtkIOCore
    )
else()
    set(VTK_LIBRARIES
        VTK::CommonDataModel
        VTK::CommonCore
        VTK::IOCore
    )
endif()


INCLUDE_DIRECTORIES(
        ${VisIt_SOURCE_DIR}
        ${VisIt_BINARY_DIR}
        ${VisIt_SOURCE_DIR}/../libtess2
        ${VTK_INCLUDE_DIRS}
)

LINK_DIRECTORIES(
    ${VisIt_BINARY_DIR}
    ${VTK_LIBRARIES}
)

file(GLOB VisItTesselator_SOURCE
    ${VisIt_SOURCE_DIR}/*.C
    ${VisIt_SOURCE_DIR}/../libtess2/*.c
)

file(GLOB VisItTesselator_HEADER
    ${VisIt_SOURCE_DIR}/*.h
    ${VisIt_SOURCE_DIR}/../libtess2/*.h
)

add_definitions(-DVisItTesselator_EXPORT)
ADD_LIBRARY(VisItTesselator SHARED ${VisItTesselator_SOURCE})
target_link_libraries(VisItTesselator ${VTK_LIBRARIES})

if (${VTK_VERSION_STRING} VERSION_GREATER_EQUAL "8.90")
    vtk_module_autoinit(TARGETS VisItTesselator MODULES ${VTK_LIBRARIES})
endif()

include(GenerateExportHeader)
generate_export_header(VisItTesselator)

#install(FILES ${VisItTesselator_HEADER} DESTINATION include)
if(WIN32)
        install(TARGETS VisItTesselator
                ARCHIVE DESTINATION lib
        )
else()
        install(TARGETS VisItTesselator LIBRARY DESTINATION lib)
endif()
