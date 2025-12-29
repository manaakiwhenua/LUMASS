PROJECT(otbsuppl)

if(RASSUPPORT)
   ADD_SUBDIRECTORY(rimageio ${otbsuppl_BINARY_DIR}/rimageio)
endif()

#ADD_SUBDIRECTORY(GDALRATImageIO ${otbsuppl_BINARY_DIR}/GDALRATImageIO)
#ADD_SUBDIRECTORY(NETCDFIO ${otbsuppl_BINARY_DIR}/NETCDFIO)
add_subdirectory(core ${otbsuppl_BINARY_DIR}/core)
ADD_SUBDIRECTORY(filters ${otbsuppl_BINARY_DIR}/filters)
