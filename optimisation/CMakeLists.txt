PROJECT(opt)

file(GLOB OPT_CPP
   ${opt_SOURCE_DIR}/*.cpp
)

file(GLOB OPT_HEADER
   ${opt_SOURCE_DIR}/NMMosra.h
)

install(FILES ${OPT_HEADER} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

