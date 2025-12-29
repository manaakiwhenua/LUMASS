PROJECT(shared)

include_directories(${QT5_INCLUDE_DIRS})
#link_directories(${QT5_LINK_DIRS})

set(CMAKE_AUTOMOC YES)
include(GenerateExportHeader)

file(GLOB SHARED_CPP 
    ${shared_SOURCE_DIR}/NMLogger.cpp 
    ${shared_SOURCE_DIR}/NMObject.cpp
)

add_definitions(-DNMShared_EXPORTS)
ADD_LIBRARY(NMShared SHARED ${SHARED_CPP})
set_property(TARGET NMShared PROPERTY LINKER_LANGUAGE CXX)
generate_export_header(NMShared)
target_link_libraries(NMShared Qt5::Core)

if (WIN32)
    install(TARGETS NMShared 
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin
    )
else()
    install(TARGETS NMShared LIBRARY DESTINATION lib)
endif()

FILE(GLOB SHARED_HEADER
        ${shared_SOURCE_DIR}/*.h
)

FILE(GLOB SHARED_SQL
        ${shared_SOURCE_DIR}/*.sql
)

file(COPY
        ${SHARED_HEADER}
        ${SHARED_SQL}
        DESTINATION ${shared_BINARY_DIR}
)

install(FILES ${SHARED_HEADER} DESTINATION include)
