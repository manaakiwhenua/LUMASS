PROJECT(otbsuppl)

if(RASSUPPORT)
   ADD_SUBDIRECTORY(rimageio ${otbsuppl_BINARY_DIR}/rimageio)
endif()

ADD_SUBDIRECTORY(GDALRATImageIO ${otbsuppl_BINARY_DIR}/GDALRATImageIO)
ADD_SUBDIRECTORY(filters ${otbsuppl_BINARY_DIR}/filters)

