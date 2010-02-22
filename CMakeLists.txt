CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5 FATAL_ERROR)

# Setup compatibility modes
IF(COMMAND cmake_policy)
	if(POLICY CMP0011)
		cmake_policy(SET CMP0011 OLD) # or NEW
	endif(POLICY CMP0011)

	cmake_policy(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)


################################################################################
# Set default values
################################################################################
PROJECT(osgPPU)
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules;${osgPPU_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")
SET(SOURCE_DIR ${osgPPU_SOURCE_DIR})
SET(OSG_DIR "${CMAKE_INSTALL_PREFIX}" CACHE STRING "Path where to find the osg installation")
SET(CUDA_DIR "${CMAKE_INSTALL_PREFIX}/cuda" CACHE STRING "Path where to find the cuda installation")


#######################################
# Versions
#######################################
SET(OSGPPU_MAJOR_VERSION 0)
SET(OSGPPU_MINOR_VERSION 5)
SET(OSGPPU_PATCH_VERSION 7)
SET(OSGPPU_SOVERSION 5)

SET(OSGPPU_RELEASE_CANDIDATE 0)

SET(OSGPPU_VERSION ${OSGPPU_MAJOR_VERSION}.${OSGPPU_MINOR_VERSION}.${OSGPPU_PATCH_VERSION})
SET(OSGPPU_SONAMES TRUE)


################################################################################
# Define already some installation components.
# This is need for later installation process if we enable it
################################################################################
IF(CMAKE_CPACK_COMMAND)
ENDIF(CMAKE_CPACK_COMMAND)


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


################################################################################
# Make the headers visible to everything
################################################################################
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



#######################################
# Solution definition
#######################################
IF(WIN32)

    IF(MSVC)
        ADD_DEFINITIONS(-D_USE_MATH_DEFINES)
        ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS)
        ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)

        # This option is to enable the /MP switch for Visual Studio 2005 and above compilers
        OPTION(WIN32_USE_MP "Set to ON to build osgPPU with the /MP option (Visual Studio 2005 and above)." OFF)
        MARK_AS_ADVANCED(WIN32_USE_MP)
        IF(WIN32_USE_MP)
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
        ENDIF(WIN32_USE_MP)

    ENDIF(MSVC)

    # Both Cygwin and Msys need -DNOMINMAX ???
    IF(UNIX)
        ADD_DEFINITIONS(-DNOMINMAX)
    ENDIF(UNIX)

	########################################################################################################
	# the following options are MSVC specific,
	# the first OSG_MSVC_VERSIONED_DLL activate a custom build-time layout that should allow to run examples and application
	# fron bin folder without requiring installation step.
	# it also prepend "osg${OPENSCENEGRAPH_SOVERSION}-" to only .dll files, leaving .lib files untouched in lib
	# it also use a hack to get rid of Debug and Release folder in MSVC projects
	# all the .dll and .pdb are in bin and all the .lib and .exp are in lib
	#
	# the second option disable incremental linking in debug build , that is enabled by default by CMake
	##########################################################################################################
    IF(MSVC)
        IF(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4 AND ${CMAKE_PATCH_VERSION} LESS 7)
            MESSAGE("Warning:  disabling versioned options 2.4.6 exibits inconsintencies in .pdb naming, at least under MSVC, suggested upgrading at least to 2.4.7")
            SET(OSGPPU_MSVC_VERSIONED_DLL OFF)
            SET(OSGPPU_MSVC_DEBUG_INCREMENTAL_LINK ON)
        ELSE(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4 AND ${CMAKE_PATCH_VERSION} LESS 7)
            OPTION(OSGPPU_MSVC_VERSIONED_DLL "Set to ON to build osgPPU with versioned dll names" ON)
            MARK_AS_ADVANCED(OSGPPU_MSVC_VERSIONED_DLL)
            OPTION(OSGPPU_MSVC_DEBUG_INCREMENTAL_LINK "Set to OFF to build without incremental linking in debug (release is off by default)" OFF)
            MARK_AS_ADVANCED(OSGPPU_MSVC_DEBUG_INCREMENTAL_LINK)
            IF(NOT OSGPPU_MSVC_DEBUG_INCREMENTAL_LINK)
                SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
                SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
                SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
            ENDIF(NOT OSGPPU_MSVC_DEBUG_INCREMENTAL_LINK)
        ENDIF(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4 AND ${CMAKE_PATCH_VERSION} LESS 7)
    ENDIF(MSVC)

ENDIF(WIN32)



################################################################################
#Solution
################################################################################
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
SET(EXAMPLE_PREFIX osgppu_ CACHE STRING "Prefix used to build examples")
LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH})


INCLUDE(OsgPPUMacroUtils)


################################################################################
# Some usefull definition
################################################################################
DETECT_OSG_VERSION()
IF(CMAKE_COMPILER_IS_GNUCC)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
ENDIF(CMAKE_COMPILER_IS_GNUCC)
IF(CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)



################################################################################
# maintainer options
################################################################################
OPTION(OSGPPU_MAINTAINER "Enable osgPPU maintainer build methods, such as making svn branches, tags, updating ChangeLog." OFF)
IF (OSGPPU_MAINTAINER)

    SET(OSGPPU_SVN "trunk")
    SET(OSGPPU_BRANCH osgPPU-${OSGPPU_MAJOR_VERSION}.${OSGPPU_MINOR_VERSION})

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
# present the packaging option only if we have the cpack command defined (effectively >= 2.6.0)
################################################################################
IF(CMAKE_CPACK_COMMAND)

	# option to build the package target
	OPTION(BUILD_OSGPPU_PACKAGES "Set to ON to generate CPack configuration files and packaging targets" OFF)

	# names used in other cmake script to determine package component
	SET(PACKAGE_HEADERS headers)
	SET(PACKAGE_LIB libraries)
	SET(PACKAGE_DATA data)
	SET(PACKAGE_SRC  source)
	SET(PACKAGE_EXAMPLES  examples)

	# we would like to build all components
	SET(CPACK_COMPONENTS_ALL ${PACKAGE_HEADERS} ${PACKAGE_LIB} ${PACKAGE_DATA} ${PACKAGE_SRC})

	# group according package properties
	SET(CPACK_COMPONENT_GROUP_DEV_DISPLAY_NAME "Development")
	SET(CPACK_COMPONENT_GROUP_RUNTIME_DISPLAY_NAME "Runtime")
	SET(CPACK_COMPONENT_GROUP_EXTRA_DISPLAY_NAME "Extras")

	# setup component properties
	SET(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C/C++ Headers")
	SET(CPACK_COMPONENT_HEADERS_DEPENDS ${PACKAGE_LIB})
	SET(CPACK_COMPONENT_HEADERS_GROUP "Development")

	SET(CPACK_COMPONENT_SOURCE_DISPLAY_NAME "C/C++ Sources")
	SET(CPACK_COMPONENT_SOURCE_DEPENDS ${PACKAGE_HEADERS})
	SET(CPACK_COMPONENT_SOURCE_DISABLED TRUE)
	SET(CPACK_COMPONENT_SOURCE_GROUP "Development")

	SET(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Runtime libraries")
	SET(CPACK_COMPONENT_LIBRARIES_REQUIRED TRUE)
	SET(CPACK_COMPONENT_LIBRARIES_GROUP "Runtime")

	SET(CPACK_COMPONENT_DATA_DISPLAY_NAME "Data files for Examples")
	SET(CPACK_COMPONENT_DATA_DISABLED TRUE)
	SET(CPACK_COMPONENT_DATA_GROUP "Extras")

	SET(CPACK_COMPONENT_EXAMPLES_DISPLAY_NAME "C/C++ Example Sources")
	SET(CPACK_COMPONENT_EXAMPLES_DEPENDS ${PACKAGE_HEADERS} ${PACKAGE_LIB} ${PACKAGE_SRC})
	SET(CPACK_COMPONENT_EXAMPLES_DISABLED TRUE)
	SET(CPACK_COMPONENT_EXAMPLES_GROUP "Extras")

	# if we would like to build them
	IF(BUILD_OSGPPU_PACKAGES)

		# include all macros which will specify target build for the packages
		INCLUDE(OsgPPUCPack)

		SET(PACKAGE_GROUP "")
		list(APPEND PACKAGE_GROUP ${PACKAGE_HEADERS})
		list(APPEND PACKAGE_GROUP ${PACKAGE_SRC})
		list(APPEND PACKAGE_GROUP ${PACKAGE_EXAMPLES})
		GENERATE_PACKAGING_GROUP_TARGET("complete")

	ENDIF(BUILD_OSGPPU_PACKAGES)
ENDIF(CMAKE_CPACK_COMMAND)


################################################################################
# Create bin and lib directories if required
################################################################################
#IF("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
#   FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib ${CMAKE_BINARY_DIR}/lib/${OSG_PLUGINS})
#ENDIF("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")


################################################################################
# Set Config file
################################################################################
SET(OSGPPU_CONFIG_HEADER "${PROJECT_BINARY_DIR}/include/osgPPU/Config.h")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/src/osgPPU/Config.in"
               "${OSGPPU_CONFIG_HEADER}")


################################################################################
### uninstall target
################################################################################
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


################################################################################
# Compile subdirectory
################################################################################
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(Data)

