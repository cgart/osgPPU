IF(WIN32)
    CMAKE_MINIMUM_REQUIRED(VERSION 2.4.6 FATAL_ERROR)
ELSE(WIN32)
    IF(APPLE)
        CMAKE_MINIMUM_REQUIRED(VERSION 2.4.6 FATAL_ERROR)
        IF(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4 AND ${CMAKE_PATCH_VERSION} LESS 7)
            MESSAGE("Warning: A critical CMake bug exists in 2.4.6 and below. Trying to build Universal Binaries will result in a compile error that seems unrelated. Either avoid building Universal Binaries by changing the CMAKE_OSX_ARCHITECTURES field to list only your architecture, or upgrade to the current CVS version of CMake or a newer stable version if it exists.")
        ENDIF(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4 AND ${CMAKE_PATCH_VERSION} LESS 7)
    ELSE(APPLE)
        CMAKE_MINIMUM_REQUIRED(VERSION 2.4.0 FATAL_ERROR)
    ENDIF(APPLE)
ENDIF(WIN32)

PROJECT(osgPPU)

SET(CMAKE_MODULE_PATH "${OpenSceneGraph_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

FIND_PACKAGE(OpenGL)

#IF(UNIX)
    # Not sure what this will do on Cygwin and Msys
    # Also, remember OS X X11 is a user installed option so it may not exist.
#    FIND_PACKAGE(X11)
    # Some Unicies need explicit linkage to the Math library or the build fails.
#    FIND_LIBRARY(MATH_LIBRARY m)
#ENDIF(UNIX)
    
# Make the headers visible to everything
INCLUDE_DIRECTORIES(
    ${OpenSceneGraph_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIR}
    ${osgPPU_SOURCE_DIR}/include
)
    
################################################################################
# Create bin and lib directories if required

IF("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
ENDIF("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")


################################################################################
# Installation stuff

SET(CMAKE_DEBUG_POSTFIX  "d")

SET(LIB_POSTFIX "")
IF(UNIX AND NOT WIN32 AND NOT APPLE)
  IF(CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET(LIB_POSTFIX "64")
  ENDIF(CMAKE_SIZEOF_VOID_P MATCHES "8")
ENDIF(UNIX AND NOT WIN32 AND NOT APPLE)

SET(OUTPUT_BINDIR ${PROJECT_BINARY_DIR}/bin)
MAKE_DIRECTORY(${OUTPUT_BINDIR})
SET(EXECUTABLE_OUTPUT_PATH ${OUTPUT_BINDIR})

SET(OUTPUT_LIBDIR ${PROJECT_BINARY_DIR}/lib)
MAKE_DIRECTORY(${OUTPUT_LIBDIR})
MAKE_DIRECTORY(${OUTPUT_LIBDIR}/osgPlugins)
SET(LIBRARY_OUTPUT_PATH ${OUTPUT_LIBDIR})

#SET(INSTALL_BINDIR OpenSceneGraph/bin)
#SET(INSTALL_INCDIR OpenSceneGraph/include)
#SET(INSTALL_LIBDIR OpenSceneGraph/lib)
#SET(INSTALL_DOCDIR OpenSceneGraph/doc)

################################################################################
# User Options

# Expose CMAKE_INCLUDE_PATH and CMAKE_LIBARY_PATH to the GUI so users
# may set these values without needing to manipulate the environment.
SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} CACHE STRING "You may add additional search paths here. Use ; to separate multiple paths.")
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} CACHE STRING "You may add additional search paths here. Use ; to separate multiple paths.")
SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} CACHE STRING "(EXPERIMENTAL) You may add additional search paths here. Use ; to separate multiple paths.")

# Dynamic vs Static Linking
OPTION(DYNAMIC_OPENSCENEGRAPH "Set to ON to build OpenSceneGraph for dynamic linking.  Use OFF for static." ON)
IF   (DYNAMIC_OPENSCENEGRAPH)
    SET(OPENSCENEGRAPH_USER_DEFINED_DYNAMIC_OR_STATIC "SHARED")
ELSE (DYNAMIC_OPENSCENEGRAPH)
    SET(OPENSCENEGRAPH_USER_DEFINED_DYNAMIC_OR_STATIC "STATIC")
ENDIF(DYNAMIC_OPENSCENEGRAPH)

#INCLUDE(OsgMacroUtils)

ADD_SUBDIRECTORY(src)

 
