# FindOSGeo4W.cmake

find_path(OSGEO4W_ROOT "OSGeo4W.bat"
	PATHS 
		c:/OSGeo4W64
		c:/OSGeo4W
		"c:/Programme Files (x86)/OSGeo4W"
		"c:/Programme Files/OSGeo4W64"
)

message(STATUS "OSGeo4W dir: ${OSGEO4W_ROOT}")

if(OSGEO4W_ROOT)
	set(EXPAT_LIBRARY ${OSGEO4W_ROOT}/lib/expat.lib)
	set(EXPAT_INCLUDE_DIR ${OSGEO4W_ROOT}/include)
	
	set(GDAL_LIBRARY ${OSGEO4W_ROOT}/lib/gdal_i.lib)
	set(GDAL_INCLUDE_DIR ${OSGEO4W_ROOT}/include)
	
	set(OpenCV_DIR ${OSGEO4W_ROOT}/share/OpenCV)

endif()

