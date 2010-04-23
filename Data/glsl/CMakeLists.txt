# --------------------------------------------------
# Here we will setup all Data files which
# has to be provided with the library
# --------------------------------------------------
SET(LIB_DATA_GLSL_FILES
	${DATA_PATH}/glsl/brightpass_fp.glsl
	${DATA_PATH}/glsl/depth_of_field_fp.glsl
	${DATA_PATH}/glsl/gauss_convolution_1Dy_fp.glsl
	${DATA_PATH}/glsl/luminance_adapted_fp.glsl
	${DATA_PATH}/glsl/luminance_mipmap_fp.glsl
	${DATA_PATH}/glsl/ssao_renderscene_fp.glsl
	${DATA_PATH}/glsl/ssao_vp.glsl
	${DATA_PATH}/glsl/bypass_fp.glsl
	${DATA_PATH}/glsl/gauss_convolution_1Dx_fp.glsl
	${DATA_PATH}/glsl/gauss_convolution_vp.glsl
	${DATA_PATH}/glsl/luminance_fp.glsl
	${DATA_PATH}/glsl/ssao_fp.glsl
	${DATA_PATH}/glsl/ssao_renderscene_vp.glsl
	${DATA_PATH}/glsl/tonemap_hdr_fp.glsl
	${DATA_PATH}/glsl/CMakeLists.txt
)

# --------------------------------------------------
# Setup how the files should be installed
# --------------------------------------------------
IF (INSTALL_DATA)
	INSTALL(
		FILES        ${LIB_DATA_GLSL_FILES}
		DESTINATION  ${INSTALL_DATADIR}/glsl
		COMPONENT    ${PACKAGE_SRC}
	)
ENDIF(INSTALL_DATA)

