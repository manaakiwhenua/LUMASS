PROJECT(shared)

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

#install(FILES ${SHARED_HEADER} DESTINATION include)
