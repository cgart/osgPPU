/*
 * Compute logarithmic luminance for next mipmap level.
 * based on: http://www.cs.utah.edu/~reinhard/cdrom/tonemap.pdf
 *
 * Additional feature:
 *   Compute adapted luminance based on the data from the previous frames.
 *   This simulated the adaption of the eye to the according luminance.
 *   The adapted luminance is stored in the alpha channel of the last mipmap level.
 */

// -------------------------------------------------------
// Texture units used for texturing
// -------------------------------------------------------
uniform sampler2D texUnit0;

uniform float g_ViewportWidth;
uniform float g_ViewportHeight;

// width of the input texture 
//uniform float g_TextureWidth;

// height of the input texture 
//uniform float g_TextureHeight;

// current mipmap level where we render the output
uniform int g_MipmapLevel;

// max and min possible luminance
uniform float maxLuminance;
uniform float minLuminance;

// time interval between two frames
uniform float invFrameTime;

// scaling factor which decides how fast to adapt for new luminance
uniform float adaptScaleFactor;


/**
 * Compute adapted luminance value.
 * @param current Is the current luminance value 
 * @param old Adapted luminance value from the previous frame
 **/
float computeAdaptedLuminance(float current, float old)
{
    // check that we do not exceed over a maximum value 
    current = clamp(current, minLuminance, maxLuminance);

    // compute new adapted value
    float lum = old + (current - old) * (1.0 - pow(0.98, adaptScaleFactor * invFrameTime));

    // clamp and return back
    return clamp(lum, minLuminance, maxLuminance);
}

/**
 * Do compute current luminance value. If we are in the last mipmap level
 * then do compute the exponent and adapted luminance value.
 **/
void main(void)
{
    const float epsilon = 0.0001f;
    float res = 0.0f;
    float c[4];
    
    // get texture sizes of the previous level
    //ivec2 size = textureSize2D(texUnit0, g_MipmapLevel - 1);
    vec2 size = vec2(g_ViewportWidth, g_ViewportHeight);

    // this is our starting sampling coordinate 
    //ivec2 iCoord = ivec2(gl_TexCoord[0] * size);
    vec2 iCoord = gl_TexCoord[0].st;
    vec2 texel = 1.0 / size;

    // create offset for the texel sampling (TODO check why -1 seems to be correct)
    //ivec2 st[4];
    vec2 st[4];
    st[0] = iCoord + ivec2(0,0);
    st[1] = iCoord + ivec2(-texel.x,0);
    st[2] = iCoord + ivec2(0,-texel.y);
    st[3] = iCoord + ivec2(-texel.x,-texel.y);
    
    // retrieve 4 texels from the previous mipmap level
    for (int i=0; i < 4; i++)
    {
        // map texels coordinates, such that they do stay in defined space
        //st[i] = clamp(st[i], 0, size.xy - 1);
        st[i] = clamp(st[i], 0, 1);
        
        // get texel from the previous mipmap level
        //c[i] = texelFetch2D(texUnit0, st[i], g_MipmapLevel - 1).r;
        c[i] = texture2DLod(texUnit0, st[i], g_MipmapLevel - 1).r;
    }

    // if we compute the first mipmap level, then just compute the sum
    // of the log values
    if (g_MipmapLevel == 1)
    {
        res += log(epsilon + c[0]);
        res += log(epsilon + c[1]);
        res += log(epsilon + c[2]);
        res += log(epsilon + c[3]);

    // for the rest we just compute the sum of underlying values
    }else{
        res += c[0];
        res += c[1];
        res += c[2];
        res += c[3];
    }

    // normalize result
    res *= 0.25;

    // if we are in the last mipmap level
    //if (g_TextureWidth < 1.001 && g_TextureHeight < 1.001)
    if (g_ViewportWidth < 1.001 && g_ViewportHeight < 1.001)
    {
        // exponentiate
        res = exp(res);
        res = min(res, 65504);

        // get old adapted luminance value 
        //float old = texelFetch2D(texUnit0, 0, g_MipmapLevel).a;
        float old = texture2DLod(texUnit0, 0, g_MipmapLevel).a;

        // compute adapted luminance value and store this
        gl_FragColor.rgb = res;
        gl_FragColor.a = computeAdaptedLuminance(res, old);
    
    // if we are not in the last mipmap level, then just store the sum
    }else{

        // result
        gl_FragColor.rgb = min(res, 65504);
        gl_FragColor.a = 1.0;
    }
}
