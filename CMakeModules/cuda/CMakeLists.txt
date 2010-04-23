# --------------------------------------------------
# Here we will setup all Data files which
# has to be provided with the library
# --------------------------------------------------
SET(LIB_CMAKE_CUDA_FILES
	CudaDependency.cmake
	empty.depend.in
	FindCuda.cmake
	make2cmake.cmake
	parse_cubin.cmake
	CMakeLists.txt
)

# --------------------------------------------------
# Setup how the files should be installed
# --------------------------------------------------
INSTALL (
	FILES ${LIB_CMAKE_CUDA_FILES}
	DESTINATION CMakeModules/cuda
	COMPONENT  ${PACKAGE_SRC}
)

