/*
 * SSAO - Technique.
 * This shader is applied on the screen sized quad with scene rendered before as input.
 */

// data containg just random values between 0 and 1
uniform vec4 fk3f[32];

// input textures with normal scene view and linearized depth values
uniform sampler2D tex0;
uniform sampler2D tex1;

// width of the input texture 
uniform float osgppu_ViewportWidth;

// height of the input texture 
uniform float osgppu_ViewportHeight;

// parameters for the ssao
const float rad = 5.0;

/**
 **/
void main(void)
{
    // this is just a resolution of the input texture
    const vec2 fres = (osgppu_ViewportWidth, osgppu_ViewportHeight);

    // get linear Z depth value
    float zLinear = texture2D( tex1, gl_TexCoord[0].xy ).z;

    // compute eye position
    vec3 ep = ez * gl_TexCoord[1].xyz / gl_TexCoord[1].z;

    vec4 pl = texture2D( tex0, gl_Color.xy*.xy );
    pl = pl*2.0 - vec4(1.0);

    float bl = 0.0;
    for( int i=0; i<32; i++ )
    {
        vec3 se = ep + rad*reflect(fk3f[i].xyz,pl.xyz);

        vec2 ss = (se.xy/se.z)*vec2(.75,1.0);
        vec2 sn = ss*.5 + vec2(.5);
        vec4 sz = texture2D(tex1,sn);

        float zd = 50.0*max( se.z-sz.x, 0.0 );
        bl += 1.0/(1.0+zd*zd);
    }
    gl_FragColor.rgb = ep.rgb;//vec4(bl/32.0);
}
