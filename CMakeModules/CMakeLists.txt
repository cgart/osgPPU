# --------------------------------------------------
# Here we setup doc direcotry
# --------------------------------------------------
SET(LIB_CMAKE_FILES

	cmake_uninstall.cmake.in
	FindOSG.cmake
	ModuleInstall.cmake
	OsgDetermineCompiler.cmake
	OsgPPUCPack.cmake
	OsgPPUCPackConfig.cmake.in
	OsgPPUMacroUtils.cmake
	CMakeLists.txt
)

# --------------------------------------------------
# Setup how the files should be installed
# --------------------------------------------------
INSTALL(
	FILES        ${LIB_CMAKE_FILES}
	DESTINATION  CMakeModules
	COMPONENT    ${PACKAGE_SRC}
)

# --------------------------------------------------
# Do the same for all subdirectories
# --------------------------------------------------
ADD_SUBDIRECTORY(cuda)
