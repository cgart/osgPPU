/*
 * Compute luminance values of the input texture.
 * So result will contain only luminance values per pixel.
 */

// -------------------------------------------------------
// Texture units used for texturing
// -------------------------------------------------------
uniform sampler2D texUnit0;


/**
 **/
void main(void)
{
	// get color from the texture
	vec4 texColor0 = texture2D(texUnit0, gl_TexCoord[0]);
	
	const float MAX_RGB = 2048.0;
	const float MAX_LUM = (0.2125f*MAX_RGB)+(0.7154*MAX_RGB)+(0.0721f*MAX_RGB);

	// compute luminance and output
	gl_FragColor.xyz = texColor0.r * 0.2125 + texColor0.g * 0.7154 + texColor0.b * 0.0721;
	//gl_FragColor.xyz /= MAX_LUM;
	gl_FragColor.a = texColor0.a;
}
