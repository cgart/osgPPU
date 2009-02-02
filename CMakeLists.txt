CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5 FATAL_ERROR)

PROJECT(osgPPU)

SET(CMAKE_MODULE_PATH "${osgPPU_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")
SET(SOURCE_DIR ${osgPPU_SOURCE_DIR})
SET(OSG_DIR "${CMAKE_INSTALL_PREFIX}" CACHE STRING "Path where to find the osg installation")
SET(CUDA_DIR "${CMAKE_INSTALL_PREFIX}/cuda" CACHE STRING "Path where to find the cuda installation")

#######################################
# Versions
#######################################
SET(OSGPPU_MAJOR_VERSION 0)
SET(OSGPPU_MINOR_VERSION 3)
SET(OSGPPU_PATCH_VERSION 9)
SET(OSGPPU_SOVERSION 3)

SET(OSGPPU_RELEASE_CANDIDATE 0)

SET(OSGPPU_VERSION ${OSGPPU_MAJOR_VERSION}.${OSGPPU_MINOR_VERSION}.${OSGPPU_PATCH_VERSION})
SET(OSGPPU_SONAMES TRUE)


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
FIND_PACKAGE( OpenGL REQUIRED )
FIND_PACKAGE( OSG REQUIRED )

#######################################
# enable CUDA support
#######################################
INCLUDE(${CMAKE_SOURCE_DIR}/CMakeModules/cuda/FindCuda.cmake)


# Make the headers visible to everything
IF(NOT ${PROJECT_BINARY_DIR} EQUAL ${PROJECT_SOURCE_DIR})
   INCLUDE_DIRECTORIES(BEFORE ${PROJECT_BINARY_DIR}/include)
ENDIF(NOT ${PROJECT_BINARY_DIR} EQUAL ${PROJECT_SOURCE_DIR})


INCLUDE_DIRECTORIES(BEFORE
    ${OPENGL_INCLUDE_DIR}
    ${OSG_INCLUDE_DIRS}
    ${SOURCE_DIR}/include
)


SET(GL_LIBS
  ${OPENGL_gl_LIBRARY}
  ${OPENGL_glu_LIBRARY}
)

# MESSAGE( "project path: ${PROJECT_BINARY_DIR}" )
# MESSAGE( "source  path: ${osgPPU_SOURCE_DIR}" )

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
SET(EXAMPLE_PREFIX osgppu_)
LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH})


INCLUDE(OsgPPUMacroUtils)


################################################################################
# Some usefull definition
################################################################################
DETECT_OSG_VERSION()
#OPTION(OSG_USE_FLOAT_MATRIX "Set to ON to build OpenSceneGraph with float Matrix instead of double." OFF)
#MARK_AS_ADVANCED(OSG_USE_FLOAT_MATRIX)
#IF(OSG_USE_FLOAT_MATRIX)
#    ADD_DEFINITIONS(-DOSG_USE_FLOAT_MATRIX)
#ENDIF(OSG_USE_FLOAT_MATRIX)

#OPTION(OSG_USE_FLOAT_PLANE "Set to ON to build OpenSceneGraph with float Plane instead of double." OFF)
#MARK_AS_ADVANCED(OSG_USE_FLOAT_PLANE)
#IF(OSG_USE_FLOAT_PLANE)
#    ADD_DEFINITIONS(-DOSG_USE_FLOAT_PLANE)
#ENDIF(OSG_USE_FLOAT_PLANE)

#OPTION(OSG_USE_FLOAT_BOUNDINGSPHERE "Set to ON to build OpenSceneGraph with float BoundingSphere instead of double." ON)
#MARK_AS_ADVANCED(OSG_USE_FLOAT_BOUNDINGSPHERE)
#IF(NOT OSG_USE_FLOAT_BOUNDINGSPHERE)
#    ADD_DEFINITIONS(-DOSG_USE_DOUBLE_BOUNDINGSPHERE)
#ENDIF(NOT OSG_USE_FLOAT_BOUNDINGSPHERE)

#OPTION(OSG_USE_FLOAT_BOUNDINGBOX "Set to ON to build OpenSceneGraph with float BoundingBox instead of double." ON)
#MARK_AS_ADVANCED(OSG_USE_FLOAT_BOUNDINGBOX)
#IF(NOT OSG_USE_FLOAT_BOUNDINGBOX)
#    ADD_DEFINITIONS(-DOSG_USE_DOUBLE_BOUNDINGBOX)
#ENDIF(NOT OSG_USE_FLOAT_BOUNDINGBOX)

IF(CMAKE_COMPILER_IS_GNUCC)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
ENDIF(CMAKE_COMPILER_IS_GNUCC)
IF(CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

################################################################################

OPTION(OSGPPU_MAINTAINER "Enable osgPPU maintainer build methods, such as making svn branches, tags, updating ChangeLog." OFF)
IF (OSGPPU_MAINTAINER)

    SET(OSGPPU_SVN "trunk")
    #SET(OPENSCENEGRAPH_SVN "branches")
    SET(OSGPPU_BRANCH OpenSceneGraph-${OSGPPU_MAJOR_VERSION}.${OSGPPU_MINOR_VERSION})

    #
    # Provide target for tagging a release
    #
    SET(SVNCOMMAND svn)
    SET(SVNTRUNKDIR     http://svn.tevs.eu/osgPPU/trunk)
    SET(SVNTAGDIR       http://svn.tevs.eu/osgPPU/tags)
    SET(SVNBRANCHDIR    http://svn.tevs.eu/osgPPU/branch)

    IF   (OSGPPU_SVN STREQUAL "trunk")
        SET(SVNSOURCEDIR ${SVNTRUNKDIR})
    ELSE (OOSGPPU_SVN STREQUAL "trunk")
        SET(SVNSOURCEDIR ${SVNBRANCH_DIR}/${OSGPPU_BRANCH})
    ENDIF(OSGPPU_SVN STREQUAL "trunk")


    IF   (OSGPPU_RELEASE_CANDIDATE EQUAL 0)
        SET(RELEASE_NAME osgPPU-${OSGPPU_VERSION})
    ELSE (OSGPPU_RELEASE_CANDIDATE EQUAL 0)
        SET(RELEASE_NAME osgPPU-${OSGPPU_VERSION}-rc${OSGPPU_RELEASE_CANDIDATE})
    ENDIF(OSGPPU_RELEASE_CANDIDATE EQUAL 0)


    ADD_CUSTOM_TARGET(tag-test
        COMMAND echo ${SVNCOMMAND} copy ${SVNSOURCEDIR} ${SVNTAGDIR}/${RELEASE_NAME} -m "Release ${RELEASE_NAME}"
    )

    ADD_CUSTOM_TARGET(tag-run
        COMMAND ${SVNCOMMAND} copy ${SVNSOURCEDIR} ${SVNTAGDIR}/${RELEASE_NAME} -m "Release ${RELEASE_NAME}"
    )

    ADD_CUSTOM_TARGET(branch-test
        COMMAND echo ${SVNCOMMAND} copy ${SVNSOURCEDIR} ${SVNBRANCHDIR}/${OSGPPU_BRANCH} -m "Branch ${OSGPPU_BRANCH}"
    )

    ADD_CUSTOM_TARGET(branch-run
        COMMAND ${SVNCOMMAND} copy ${SVNSOURCEDIR} ${SVNBRANCHDIR}/${OSGPPU_BRANCH} -m "Branch ${OSGPPU_BRANCH}"
    )

    #
    # Provide target for generating ChangeLog
    #
    SET(GENERATELOGS svn2cl)

    ADD_CUSTOM_TARGET(ChangeLog
        COMMAND ${SVNCOMMAND} update
        #COMMAND ${SVNCOMMAND} up
        #COMMAND ${GENERATELOGS}
        COMMAND ${SVNCOMMAND} log --xml --verbose | xsltproc doc/scripts/svn2cl.xsl - > ChangeLog
    )
    
ENDIF(OSGPPU_MAINTAINER)


################################################################################
# Create bin and lib directories if required
################################################################################
#IF("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
#   FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib ${CMAKE_BINARY_DIR}/lib/${OSG_PLUGINS})
#ENDIF("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")


################################################################################
# Compile subdirectory
################################################################################
IF(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

ADD_SUBDIRECTORY(src)


################################################################################
# Set Config file
################################################################################
SET(OSGPPU_CONFIG_HEADER "${PROJECT_BINARY_DIR}/include/osgPPU/Config.h")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/src/osgPPU/Config.in"
               "${OSGPPU_CONFIG_HEADER}")
# INSTALL_FILES(/include/osg/ FILES "${OPENSCENEGRAPH_CONFIG_HEADER}")


################################################################################
### uninstall target
################################################################################
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


