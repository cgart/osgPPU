SET(TARGET_TARGETNAME
    ${EXAMPLE_PREFIX}cuda
)

SET(TARGET_SRC 
    cuda.cpp
)

SET(TARGET_H 
    osgteapot.h
)

#### end var setup  ###

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

#------------------------------------
# CUDA module project UnitInOutModule
#------------------------------------
# Add current directory to the nvcc include line.
CUDA_INCLUDE_DIRECTORIES(
    ${OPENGL_INCLUDE_DIR}
    ${OSG_INCLUDE_DIRS}
    ${SOURCE_DIR}/include
)

# Link cuda code in a library to something else.
CUDA_ADD_LIBRARY(osgppu_cudakernel
  kernel.cu
  ProcessingModule.cpp
  Export.h
  BlurKernelWrapper.h
  cutil.h
  )

# Link cuda with cutil library and with internals of osgPPU
#TARGET_LINK_LIBRARIES(osgppu_cudakernel ${CUDA_CUT_TARGET_LINK} )
LINK_INTERNAL(osgppu_cudakernel osgPPU)

