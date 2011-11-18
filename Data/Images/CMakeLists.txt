# --------------------------------------------------
# Here we will setup all Data files which
# has to be provided with the library
# --------------------------------------------------
SET(LIB_DATA_IMAGES_FILES
	${DATA_PATH}/Images/lz.rgb
	${DATA_PATH}/Images/reflect.rgb
	${DATA_PATH}/Images/skymap.jpg
	${DATA_PATH}/Images/tank.rgb
	${DATA_PATH}/Images/video.avi
	${DATA_PATH}/Images/CMakeLists.txt
)

# --------------------------------------------------
# Setup how the files should be installed
# --------------------------------------------------
IF (INSTALL_DATA)
	INSTALL(
		FILES        ${LIB_DATA_IMAGES_FILES}
		DESTINATION  ${INSTALL_DATADIR}/Images
#		COMPONENT    ${PACKAGE_DATA}
 	)
ENDIF(INSTALL_DATA)

