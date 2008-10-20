/*
 * SSAO - Technique.
 * This shader is applied on the screen sized quad with scene rendered before as input.
 */

// data containg just random values between 0 and 1
uniform vec4 fk3f[32];

// input textures with normal scene view and linearized depth values
uniform sampler2D texColorMap;
uniform sampler2D texDepthMap;

// width of the input texture 
uniform float osgppu_ViewportWidth;

// height of the input texture 
uniform float osgppu_ViewportHeight;

// parameters for the ssao
const float rad = 0.5;

// zNear and zFar values 
uniform float zNear;
uniform float zFar;

/**
 * Convert z-depth value into camera distance coordinate
 **/
float convertZ(in float depth)
{
    // compute distance to the viewer
    float a = zFar / ( zFar - zNear );
    float b = zFar * zNear / ( zNear - zFar );    
    float dist = b / ( depth - a );

    return (dist - zNear) / (zFar - zNear);
}


/**
 **/
void main(void)
{
    // this is just a resolution of the input texture
    const vec2 fres = (osgppu_ViewportWidth, osgppu_ViewportHeight);

    // get pixel depth value
    float depth = texture2D(texDepthMap, gl_TexCoord[0].xy).z;

    // do work only on fragments with valud depth value
    if (depth < 0.99999)
    {
        // get linear Z depth value
        float zLinear = convertZ(depth);
    
        // compute eye position
        vec3 ep = zLinear * gl_TexCoord[1].xyz;// / gl_TexCoord[1].z;    
        vec3 bl = vec3(0,0,0);

        // for each sample we do
        for (int i=0; i < 1; i++)
        {
            // compute random vector pointing from eye to some point in the scene within certain radius
            vec3 se = ep + rad * fk3f[i].xyz;
    
            // project the point back into the screen to get the screen coordinates
            vec3 ss = se.xyz * vec3(.75,1.0,1.0);
            vec4 sz = texture2DProj( tex0, ss*.5+ss.z*vec3(.5) );

            //vec2 ss = (se.xy/se.z)*vec2(.75,1.0);
            //vec2 sn = ss*.5 + vec2(.5);
            //vec4 sz = texture2D(texDepthMap, sn);

            float zd = 50.0*max( se.z-sz.x, 0.0 );
            bl += se;//1.0/(1.0+zd*zd);              // occlusion = 1/( 1 + 2500*max{dist,0)^2 )
        }
    
        /*vec4 pl = texture2D( tex0, gl_Color.xy*.xy );
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
        }*/
        gl_FragColor.rgb = bl;//vec3(bl/32.0);
    }else
        gl_FragColor.rgb = vec3(0,0,0);
}
