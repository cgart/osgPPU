# --------------------------------------------------
# Here we will setup all Data files which
# has to be provided with the library
# --------------------------------------------------
SET(LIB_DOC_SCRIPTS_FILES
	svn2cl.xsl
	CMakeLists.txt
)

# --------------------------------------------------
# Setup how the files should be installed
# --------------------------------------------------
INSTALL (
	FILES ${LIB_DOC_SCRIPTS_FILES}
	DESTINATION doc/scripts
	COMPONENT  ${PACKAGE_SRC}
)

