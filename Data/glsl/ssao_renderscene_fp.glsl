/*
 * Just render the scene in an ususal way. Output linear z-depth values
 * in the second texture coordiantes.
 */


/**
 **/
void main(void)
{
   // select a color based on face
   gl_FragData[0] = gl_Color.rgb;

   // output z-depth values in a scond mrt
   gl_FragData[1] = gl_TexCoord[1];
}
