IF(DYNAMIC_OSGPPU)
    ADD_DEFINITIONS(-DOSGPPU_LIBRARY)
ELSE(DYNAMIC_OSGPPU)
    ADD_DEFINITIONS(-DOSGPPU_LIBRARY_STATIC)
ENDIF(DYNAMIC_OSGPPU)

SET(LIB_NAME ${PROJECT_NAME})
SET(HEADER_PATH ${osgPPU_SOURCE_DIR}/include/${LIB_NAME})

#-----------------------------------
# Setup headers
#-----------------------------------
SET(LIB_PUBLIC_HEADERS
    ${HEADER_PATH}/Export.h
    ${HEADER_PATH}/UnitText.h
    ${HEADER_PATH}/UnitInOut.h
    ${HEADER_PATH}/UnitInResampleOut.h
    ${HEADER_PATH}/UnitInMipmapOut.h
    ${HEADER_PATH}/UnitMipmapInMipmapOut.h
    ${HEADER_PATH}/UnitOut.h
    ${HEADER_PATH}/UnitOutCapture.h
    ${HEADER_PATH}/Processor.h
    ${HEADER_PATH}/Unit.h
    ${HEADER_PATH}/UnitBypass.h
    ${HEADER_PATH}/UnitDepthbufferBypass.h
    ${HEADER_PATH}/UnitCameraAttachmentBypass.h
    ${HEADER_PATH}/UnitTexture.h
    ${HEADER_PATH}/Visitor.h
    ${HEADER_PATH}/BarrierNode.h
    ${HEADER_PATH}/Utility.h
    ${HEADER_PATH}/ColorAttribute.h
    ${HEADER_PATH}/ShaderAttribute.h
    ${HEADER_PATH}/UnitCamera.h
    ${HEADER_PATH}/UnitInHistoryOut.h
    ${HEADER_PATH}/UnitInOutModule.h
    ${HEADER_PATH}/UnitBypassRepeat.h
    ${OSGPPU_CONFIG_HEADER}
)

#-----------------------------------
# Setup source files
#-----------------------------------
SET(LIB_SRC_FILES
    Unit.cpp
    UnitBypass.cpp
    UnitDepthbufferBypass.cpp
    UnitCameraAttachmentBypass.cpp
    UnitTexture.cpp
    UnitOut.cpp
    UnitOutCapture.cpp
    UnitInOut.cpp
    UnitText.cpp
    UnitInResampleOut.cpp
    UnitInMipmapOut.cpp
    UnitMipmapInMipmapOut.cpp
    Processor.cpp
    Visitor.cpp
    Utility.cpp
    ColorAttribute.cpp
    ShaderAttribute.cpp
    UnitCamera.cpp
    UnitInOutModule.cpp
	CMakeLists.txt
    UnitInHistoryOut.cpp
    UnitBypassRepeat.cpp
)


#-----------------------------------
# Create library command, combines headers and sources
#-----------------------------------
ADD_LIBRARY(${LIB_NAME}
    ${OSGPPU_USER_DEFINED_DYNAMIC_OR_STATIC}
    ${LIB_PUBLIC_HEADERS}
    ${LIB_SRC_FILES}
)


#-----------------------------------
# Link other libraries
#-----------------------------------
LINK_WITH_VARIABLES(${LIB_NAME}     
    OSG_LIBRARY
    OSGDB_LIBRARY
    OSGTEXT_LIBRARY
    OSGUTIL_LIBRARY
    OPENTHREADS_LIBRARY
)
LINK_EXTERNAL(${LIB_NAME} ${OPENGL_LIBRARIES}) 
LINK_CORELIB_DEFAULT(${LIB_NAME})

#-----------------------------------
# Some definitions for debug and msvc
#-----------------------------------
SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES DEBUG_POSTFIX "d")
if(MSVC)
    SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES PREFIX "../")
    SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES IMPORT_PREFIX "../")
endif(MSVC)


#-----------------------------------
# Include install module
#-----------------------------------
INCLUDE(ModuleInstall OPTIONAL)
