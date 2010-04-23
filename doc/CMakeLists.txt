# --------------------------------------------------
# Here we setup doc direcotry
# --------------------------------------------------
SET(LIB_DOC_FILES

	config.doxy  
	createChangelog.sh
	footer.html
	Makefile
	CMakeLists.txt
)

# --------------------------------------------------
# Setup how the files should be installed
# --------------------------------------------------
INSTALL(
	FILES        ${LIB_DOC_FILES}
	DESTINATION  doc
	COMPONENT    ${PACKAGE_SRC}
)

# --------------------------------------------------
# Do the same for all subdirectories
# --------------------------------------------------
ADD_SUBDIRECTORY(scripts)
