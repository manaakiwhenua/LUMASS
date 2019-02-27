PROJECT(VisIt)

find_package(VTK COMPONENTS
        vtkCommonDataModel
        vtkCommonCore
        vtkIOCore
)


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


#if(WIN32)
#    ADD_LIBRARY(VisItTesselator SHARED )
#else()
    ADD_LIBRARY(VisItTesselator SHARED ${VisItTesselator_SOURCE})
    target_link_libraries(VisItTesselator
            vtkCommonDataModel
            vtkCommonCore
            vtkIOCore
    )
#endif()

include(GenerateExportHeader)
generate_export_header(VisItTesselator)

install(FILES  ${VisItTesselator_HEADER} DESTINATION include)
if(WIN32)
#        install(TARGETS VisItTesselator
#                LIBRARY DESTINATION lib
#                RUNTIME DESTINATION lib
#                ARCHIVE DESTINATION lib
#        )

#        install(TARGETS VisItTesselator
#                LIBRARY DESTINATION lib
#                RUNTIME DESTINATION lib
#        )


        install(TARGETS VisItTesselator
                ARCHIVE DESTINATION lib
        )
else()
        install(TARGETS VisItTesselator LIBRARY DESTINATION lib)
endif()
