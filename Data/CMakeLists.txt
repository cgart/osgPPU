# --------------------------------------------------
# Here we will setup all Data files which
# has to be provided with the library
# --------------------------------------------------
#SET(LIB_NAME ${PROJECT_NAME})

OPTION(INSTALL_DATA "Should data from Data/ directory be also installed" OFF)
SET(INSTALL_DATADIR "Data" CACHE STRING "Relative path where data will be installed.")

SET(DATA_PATH ${osgPPU_SOURCE_DIR}/Data)

SET(LIB_DATA_FILES

	${DATA_PATH}/bypass.ppu  
	${DATA_PATH}/cessnafire.osg  
	${DATA_PATH}/cow.osg
	${DATA_PATH}/cuda.ppu
	${DATA_PATH}/dof.ppu
	${DATA_PATH}/hdr.ppu
	${DATA_PATH}/motionblur.ppu
	${DATA_PATH}/temple.ive
	${DATA_PATH}/CMakeLists.txt
)

# --------------------------------------------------
# Setup how the files should be installed
# --------------------------------------------------
IF (INSTALL_DATA)
	INSTALL(
		FILES        ${LIB_DATA_FILES}
		DESTINATION  ${INSTALL_DATADIR}
		COMPONENT    ${PACKAGE_SRC}
	)
ENDIF(INSTALL_DATA)

# --------------------------------------------------
# Do the same for all subdirectories
# --------------------------------------------------
ADD_SUBDIRECTORY(glsl)
ADD_SUBDIRECTORY(Images)


# --------------------------------------------------
# Setup file group for Visual Studio
# --------------------------------------------------
SET(DATA_GROUP "Data Files")
SOURCE_GROUP(
    ${DATA_GROUP}
    FILES ${LIB_DATA_FILES} ${LIB_DATA_GLSL_FILES} ${LIB_DATA_IMAGES_FILES}
)
