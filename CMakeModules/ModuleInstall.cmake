# INSTALL and SOURCE_GROUP commands for osgPPU

# Required Vars:
# ${LIB_NAME}
# ${LIB_PUBLIC_HEADERS}

# --------------------------------------------------
# Setup destination directories
# --------------------------------------------------
SET(INSTALL_INCDIR include)
SET(INSTALL_BINDIR bin)
SET(INSTALL_SRCDIR src)
IF(WIN32)
    SET(INSTALL_LIBDIR bin)
    SET(INSTALL_ARCHIVEDIR lib)
ELSE(WIN32)
    SET(INSTALL_LIBDIR lib)
    SET(INSTALL_ARCHIVEDIR lib)
ENDIF(WIN32)

IF(MSVC)
	HANDLE_MSVC_DLL()
ENDIF(MSVC)

INSTALL(
    TARGETS ${LIB_NAME}
    RUNTIME DESTINATION ${INSTALL_BINDIR}
    LIBRARY DESTINATION ${INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR}
)


# --------------------------------------------------
# Setup header file group for Visual Studio
# --------------------------------------------------
SET(HEADERS_GROUP "Header Files")
SOURCE_GROUP(
    ${HEADERS_GROUP}
    FILES ${LIB_PUBLIC_HEADERS}
)

# --------------------------------------------------
# Setup source file group for Visual Studio
# --------------------------------------------------
SET(SRC_GROUP "Source Files")
SOURCE_GROUP(
    ${SRC_GROUP}
    FILES ${LIB_SRC_FILES}
)


# --------------------------------------------------
# Install routines for differnet components
# FIXME: Do not run for OS X framework
# --------------------------------------------------
INSTALL(
    FILES        ${LIB_PUBLIC_HEADERS}
    DESTINATION  ${INSTALL_INCDIR}/${LIB_NAME}
    COMPONENT    ${PACKAGE_HEADERS} 
)

INSTALL(
    FILES        ${LIB_SRC_FILES}
    DESTINATION  ${INSTALL_SRCDIR}/${LIB_NAME}
    COMPONENT    ${PACKAGE_SRC}
)

#-----------------------------------------------------------
# Include the build system parts to the source package
#-----------------------------------------------------------
INSTALL (
	FILES
		${PROJECT_SOURCE_DIR}/CMakeLists.txt
		${PROJECT_SOURCE_DIR}/ChangeLog
		${PROJECT_SOURCE_DIR}/CONTRIBUTORS.txt
		${PROJECT_SOURCE_DIR}/LICENSE.txt
	DESTINATION ./
	COMPONENT  ${PACKAGE_SRC}
)

INSTALL (
	FILES
		${PROJECT_SOURCE_DIR}/src/CMakeLists.txt
	DESTINATION src
	COMPONENT  ${PACKAGE_SRC}
)

INSTALL (
	FILES
		${PROJECT_SOURCE_DIR}/src/osgPlugins/CMakeLists.txt
	DESTINATION src/osgPlugins
	COMPONENT  ${PACKAGE_SRC}
)
