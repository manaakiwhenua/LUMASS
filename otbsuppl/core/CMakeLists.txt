PROJECT(otbsuppl_core)

FILE(GLOB SHARED_HEADER
        ${shared_SOURCE_DIR}/*.h
)

file(COPY
        ${SHARED_HEADER}
        DESTINATION ${shared_BINARY_DIR}
)

#install(FILES ${SHARED_HEADER} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
