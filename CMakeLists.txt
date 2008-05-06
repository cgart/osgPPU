CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5 FATAL_ERROR)

PROJECT(osgPPU)

SET(CMAKE_MODULE_PATH "${osgPPU_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")
SET(SOURCE_DIR ${osgPPU_SOURCE_DIR})
SET(OSG_VERSION "2.4.0" CACHE STRING "Version of the OpenSceneGraph for which the osgdb_ppu plugin is built")
SET(OSG_LIBRARY_DIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE STRING "Path where to find the osg libraries")


#######################################
# Dynamic vs Static Linking
#######################################
OPTION(DYNAMIC_OSGPPU "Set to ON to build osgPPU for dynamic linking.  Use OFF for static." ON)
IF (DYNAMIC_OSGPPU)
    SET(OSGPPU_USER_DEFINED_DYNAMIC_OR_STATIC "SHARED")
ELSE (DYNAMIC_OSGPPU)
    SET(OSGPPU_USER_DEFINED_DYNAMIC_OR_STATIC "STATIC")
    SET(CMAKE_MODULE_CXX_FLAGS_DEBUG "-pthread")
ENDIF(DYNAMIC_OSGPPU)


#######################################
# Library files
#######################################
FIND_PACKAGE(OpenGL)
FIND_PACKAGE(OSG)
FIND_PACKAGE(OpenThreads)

INCLUDE_DIRECTORIES(BEFORE
    ${OPENGL_INCLUDE_DIR}
    ${OSG_INCLUDE_DIR}
    ${SOURCE_DIR}/include 
)

SET(GL_LIBS 
  ${OPENGL_gl_LIBRARY} 
  ${OPENGL_glu_LIBRARY}
)


#######################################
# Solution definition
#######################################
IF(WIN32)
    ADD_DEFINITIONS(-D_USE_MATH_DEFINES)
    ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)

    IF(MSVC)
        OPTION(OSGPPU_MSVC_DEBUG_INCREMENTAL_LINK "Set to OFF to build without incremental linking in debug (release is off by default)" OFF)
        IF(NOT OSGPPU_MSVC_DEBUG_INCREMENTAL_LINK)
            SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
            SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
            SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
        ENDIF(NOT OSGPPU_MSVC_DEBUG_INCREMENTAL_LINK)
    ENDIF(MSVC)
ENDIF(WIN32)



#Solution
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH})


INCLUDE(OsgPPUMacroUtils)


################################################################################
# Create bin and lib directories if required
################################################################################
#IF("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
#   FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib ${CMAKE_BINARY_DIR}/lib/${OSG_PLUGINS})
#ENDIF("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")


################################################################################
# Compile subdirectory
################################################################################
ADD_SUBDIRECTORY(src)


################################################################################
### uninstall target
################################################################################
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


