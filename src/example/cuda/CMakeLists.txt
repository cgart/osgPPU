#---------------------------------------
# CUDA Module build for the exmaple
#---------------------------------------
# Add current directory to the nvcc include line.
CUDA_INCLUDE_DIRECTORIES(
    ${OPENGL_INCLUDE_DIR}
    ${OSG_INCLUDE_DIRS}
    ${SOURCE_DIR}/include
)

# Setup the prefix of the generated cuda file
SET(CMAKE_SHARED_MODULE_PREFIX "osgppu_")

# Setup header files for the cuda library
SET(CUDA_MODULE_HEADERS
	 Export.h
)

# Link cuda code in a library to something else.
SET(CUDA_MODULE_SRC
  kernel.cu
  ProcessingModule.cpp
  Export.h
  BlurKernelWrapper.h
  osgteapot.h
)
CUDA_ADD_LIBRARY(cudakernel
	${CUDA_MODULE_SRC}
)

# Link with osgPPU
LINK_INTERNAL(cudakernel osgPPU)

# Link with other libraries
LINK_WITH_VARIABLES(cudakernel   
    OSG_LIBRARY
    OPENTHREADS_LIBRARY
)

# Link with OpenGL
LINK_EXTERNAL(cudakernel ${OPENGL_LIBRARIES}) 


#---------------------------------------
# Example build
#---------------------------------------
SET(TARGET_TARGETNAME
    ${EXAMPLE_PREFIX}cuda
)

SET(TARGET_SRC 
    cuda.cpp
)
SET(TARGET_H 
    osgteapot.h
)


ADD_EXECUTABLE(${TARGET_TARGETNAME} ${TARGET_SRC} ${TARGET_H})
LINK_INTERNAL(${TARGET_TARGETNAME} osgPPU)
LINK_WITH_VARIABLES(${TARGET_TARGETNAME}     
    OSGVIEWER_LIBRARY
    OSGDB_LIBRARY
    OSGGA_LIBRARY
    OSGTEXT_LIBRARY
    OSGUTIL_LIBRARY
    OSG_LIBRARY
    OPENTHREADS_LIBRARY
)

LINK_EXTERNAL(${TARGET_TARGETNAME} ${OPENGL_LIBRARIES}) 

IF (NOT DYNAMIC_OSGPPU)
    LINK_EXTERNAL(${TARGET_TARGETNAME} pthread) 
ENDIF(NOT DYNAMIC_OSGPPU)

SET_TARGET_PROPERTIES(${TARGET_TARGETNAME} PROPERTIES DEBUG_POSTFIX "d")
if(MSVC)
	SET_TARGET_PROPERTIES(${TARGET_TARGETNAME} PROPERTIES PREFIX "../")
	SET_TARGET_PROPERTIES(${TARGET_TARGETNAME} PROPERTIES PROJECT_LABEL "Example ${TARGET_TARGETNAME}")
endif(MSVC)

#-----------------------------------------------
# Add the file to the install target
#-----------------------------------------------
INSTALL (
	FILES
		CMakeLists.txt
		${TARGET_SRC}
		${TARGET_H}
		${CUDA_MODULE_HEADERS}
		${CUDA_MODULE_SRC}
	DESTINATION src/examples/cuda
	COMPONENT  ${PACKAGE_EXAMPLES}
)
