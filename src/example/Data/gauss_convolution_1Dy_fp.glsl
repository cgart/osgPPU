/*
 * Apply convolution of variable size onto the pixels.
 * The convolution is done in 1D
 */

// -------------------------------------------------------
// Texture units used for texturing
// -------------------------------------------------------
uniform sampler2D texUnit0;

//! Size of the gaussian kernel (size = radius * 2)
uniform float radius;

//! Sigam value for the gaussian kernel
uniform float sigma;

// Varyings
varying float sigma2;
varying float c;

// width of the input texture 
//uniform float g_TextureWidth[16];

// height of the input texture 
//uniform float g_TextureHeight[16];

/**
 **/
void main(void)
{
    // get texture size 
    float g_TextureWidth = textureSize2D(texUnit0, 0).x;
    float g_TextureHeight = textureSize2D(texUnit0, 0).y;

	// store here resulting color
	vec4 color;
	float totalWeigth = 0;
	float inputTexTexelWidth = 1.0 / g_TextureHeight;

	// convolve by applying nsamples-time the texture lookup
	for (float i=-radius; i < radius; i += 1.0) 
	{
		// compute weight for the pixel 
		float weight = c * exp((i*i) / (-sigma2));
		totalWeigth += weight;
		
		// combine now the sum as all values multiplied by the weight
		color += texture2D(texUnit0, gl_TexCoord[0].xy +  vec2(0, i * inputTexTexelWidth) ) * weight;
	}
	color /= totalWeigth;
	
	gl_FragColor = color;
}
