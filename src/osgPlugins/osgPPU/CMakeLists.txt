PROJECT(OSG_PLUGINS_MASTER)

#######################################
# Plugin prefix
#######################################
SET(OSG_PLUGIN_PREFIX "")
IF (CYGWIN)
    SET(OSG_PLUGIN_PREFIX "cygwin_")
ENDIF(CYGWIN)
IF(MINGW)
    SET(OSG_PLUGIN_PREFIX "mingw_")
ENDIF(MINGW)


###########################################
# Settings for plugin
###########################################
IF(NOT MSVC)
    SET(LIBRARY_OUTPUT_PATH "${LIBRARY_OUTPUT_PATH}/${OSG_PLUGINS}")
ENDIF(NOT MSVC)

IF(MSVC80)
  IF(NOT OSG_MSVC_GENERATE_PLUGINS_AND_WRAPPERS_MANIFESTS)
    SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /MANIFEST:NO")
  ENDIF(NOT OSG_MSVC_GENERATE_PLUGINS_AND_WRAPPERS_MANIFESTS)
ENDIF(MSVC80)

SET(CMAKE_SHARED_MODULE_PREFIX ${OSG_PLUGIN_PREFIX})
SET(TARGET_DEFAULT_PREFIX "osgdb_")
SET(TARGET_DEFAULT_LABEL_PREFIX "Plugins")



###########################################
# Generate plugin
###########################################
SET(TARGET_SRC 
    ReaderWriterPPU.cpp
    IO_Unit.cpp
    Base.cpp
    Base.h
)
SET(TARGET_ADDED_LIBRARIES osgPPU )
SETUP_PLUGIN(ppu)





