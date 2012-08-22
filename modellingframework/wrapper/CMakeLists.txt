PROJECT(mfw_wrapper)

file(GLOB MFW_WRAPPER_H
  ${mfw_wrapper_SOURCE_DIR}/*.h
)

file(GLOB MFW_WRAPPER_CPP
  ${mfw_wrapper_SOURCE_DIR}/*.cpp
)

# when we're not in rasdaman mode, remove related files 
if (NOT RASSUPPORT)
  list(REMOVE_ITEM MFW_WRAPPER_H 
  	"${mfw_wrapper_SOURCE_DIR}/NMRasdamanConnectorWrapper.h"
  )
  
  list(REMOVE_ITEM MFW_WRAPPER_CPP
        "${mfw_wrapper_SOURCE_DIR}/NMRasdamanConnectorWrapper.cpp"
  )
endif()
