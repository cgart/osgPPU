IF(DYNAMIC_OSGPPU)
    ADD_DEFINITIONS(-DOSGPPU_LIBRARY)
ELSE(DYNAMIC_OSGPPU)
    ADD_DEFINITIONS(-DOSGPPU_LIBRARY_STATIC)
ENDIF(DYNAMIC_OSGPPU)

SET(LIB_NAME osgPPU)
SET(HEADER_PATH ${osgPPU_SOURCE_DIR}/include/${LIB_NAME})

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
    ${HEADER_PATH}/Shader.h
    ${HEADER_PATH}/Unit.h
    ${HEADER_PATH}/UnitBypass.h
    ${HEADER_PATH}/UnitDepthbufferBypass.h
    ${HEADER_PATH}/UnitTexture.h
    ${HEADER_PATH}/Visitor.h
    ${HEADER_PATH}/BarrierNode.h
    ${HEADER_PATH}/Utility.h
    ${HEADER_PATH}/ColorAttribute.h
)

ADD_LIBRARY(${LIB_NAME}
    ${OSGPPU_USER_DEFINED_DYNAMIC_OR_STATIC}
    ${LIB_PUBLIC_HEADERS}
    Unit.cpp
    UnitBypass.cpp
    UnitDepthbufferBypass.cpp
    UnitTexture.cpp
    UnitOut.cpp
    UnitOutCapture.cpp
    UnitInOut.cpp
    UnitText.cpp
    UnitInResampleOut.cpp
    UnitInMipmapOut.cpp
    UnitMipmapInMipmapOut.cpp
    Shader.cpp
    Processor.cpp
    Visitor.cpp
    Utility.cpp
    ColorAttribute.cpp
)

LINK_WITH_VARIABLES(${LIB_NAME}     
    OSG_LIBRARY
    OSGDB_LIBRARY
    OSGTEXT_LIBRARY
    OSGUTIL_LIBRARY
)
LINK_CORELIB_DEFAULT(${LIB_NAME})

SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES DEBUG_POSTFIX "d")
if(MSVC)
	SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES PREFIX "../")
	SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES IMPORT_PREFIX "../")
endif(MSVC)

INCLUDE(ModuleInstall OPTIONAL)
