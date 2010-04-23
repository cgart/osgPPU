ADD_SUBDIRECTORY(hdr)
ADD_SUBDIRECTORY(viewer)
ADD_SUBDIRECTORY(dof)
ADD_SUBDIRECTORY(cubemap)
ADD_SUBDIRECTORY(texture3D)
ADD_SUBDIRECTORY(video)
ADD_SUBDIRECTORY(ssao)
ADD_SUBDIRECTORY(glow)
ADD_SUBDIRECTORY(motionblur)
ADD_SUBDIRECTORY(blurScene)

#if CUDA found, then build cuda example
IF(CUDA_BUILD_EXAMPLES AND CUDA_NVCC)
    ADD_SUBDIRECTORY(cuda)
ENDIF(CUDA_BUILD_EXAMPLES AND CUDA_NVCC)

#-----------------------------------------------
# Add the file to the install target
#-----------------------------------------------
INSTALL (
	FILES
		CMakeLists.txt
	DESTINATION src/examples
	COMPONENT  ${PACKAGE_EXAMPLES}
)

################################################################################
# Copy data files out from src/examples into the build bin directory
################################################################################
#FILE(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Data" "${CMAKE_BINARY_DIR}/bin/Data/glsl" "${CMAKE_BINARY_DIR}/bin/Data/Images")
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/dof.ppu" "${CMAKE_BINARY_DIR}/bin/Data/" COPYONLY)
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/hdr.ppu" "${CMAKE_BINARY_DIR}/bin/Data/" COPYONLY)
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/motionblur.ppu" "${CMAKE_BINARY_DIR}/bin/Data/" COPYONLY)
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/bypass.ppu" "${CMAKE_BINARY_DIR}/bin/Data/" COPYONLY)
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/cow.osg" "${CMAKE_BINARY_DIR}/bin/Data/" COPYONLY)
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/cessnafire.osg" "${CMAKE_BINARY_DIR}/bin/Data/" COPYONLY)
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/Images/reflect.rgb" "${CMAKE_BINARY_DIR}/bin/Data/Images/" COPYONLY)

#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/glsl/brightpass_fp.glsl" "${CMAKE_BINARY_DIR}/bin/Data/glsl/" COPYONLY)
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/glsl/bypass_fp.glsl" "${CMAKE_BINARY_DIR}/bin/Data/glsl/" COPYONLY)
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/glsl/depth_of_field_fp.glsl" "${CMAKE_BINARY_DIR}/bin/Data/glsl/" COPYONLY)
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/glsl/gauss_convolution_1Dy_fp.glsl" "${CMAKE_BINARY_DIR}/bin/Data/glsl/" COPYONLY)
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/glsl/gauss_convolution_1Dx_fp.glsl" "${CMAKE_BINARY_DIR}/bin/Data/glsl/" COPYONLY)
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/glsl/gauss_convolution_vp.glsl" "${CMAKE_BINARY_DIR}/bin/Data/glsl/" COPYONLY)
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/glsl/luminance_adapted_fp.glsl" "${CMAKE_BINARY_DIR}/bin/Data/glsl/" COPYONLY)
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/glsl/luminance_fp.glsl" "${CMAKE_BINARY_DIR}/bin/Data/glsl/" COPYONLY)
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/glsl/luminance_mipmap_fp.glsl" "${CMAKE_BINARY_DIR}/bin/Data/glsl/" COPYONLY)
#CONFIGURE_FILE("${SOURCE_DIR}/src/example/Data/glsl/tonemap_hdr_fp.glsl" "${CMAKE_BINARY_DIR}/bin/Data/glsl/" COPYONLY)


