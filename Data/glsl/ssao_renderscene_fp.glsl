/*
 * Just render the scene in an ususal way. Output linear z-depth values
 * in the second texture coordiantes.
 */

// zNear and zFar values 
uniform float zNear;
uniform float zFar;

/**
 **/
void main(void)
{
   // select a color based on face
   gl_FragData[0] = gl_Color;

   // output z-depth values in a scond mrt
   gl_FragData[1] = gl_TexCoord[1];
   gl_FragData[1].z /= zFar;
}
