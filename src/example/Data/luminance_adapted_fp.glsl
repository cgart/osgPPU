/*
 * Compute adapted luminance based on the data from the previous frames.
 * based on: http://www.cs.utah.edu/~reinhard/cdrom/tonemap.pdf
 */

// -------------------------------------------------------
// Texture units used for texturing
// -------------------------------------------------------
// input texture containing the average luminance
uniform sampler2D texLuminance;

// input texture containing the current adapted luminance
uniform sampler2D texAdaptedLuminance;

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
void main(void)
{
    // get current luminance, this one is stored in the last mipmap level
    float current = texture2D(texLuminance, vec2(0,0), 100.0).x;
    
    // get old adapted luminance value
    float old = texture2D(texAdaptedLuminance, vec2(0,0)).x;

    // check that we do not exceed over a maximum value 
    //current = clamp(current, minLuminance, maxLuminance);

    // compute new adapted value
    float lum = old + (current - old) * (1.0 - pow(0.98, adaptScaleFactor * invFrameTime));

    // clamp and return back
    gl_FragData[0].xyz = maxLuminance;//clamp(lum, minLuminance, maxLuminance);
    gl_FragData[0].a = 1.0;
}
