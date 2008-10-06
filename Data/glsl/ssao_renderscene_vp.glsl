/*
 * Just render the scene in an ususal way. Output linear z-depth values
 * in the second texture coordiantes.
 */


/**
 **/
void main(void)
{
   // bypass the texture coordinate data
   gl_TexCoord[0] = gl_MultiTexCoord0;

   // compute position of the pixel
   gl_Position = ftransform();

   // bypass color data
   gl_FrontColor = gl_Color;

   // compute linear depth value
   // this is just a simple matrix multiplication
   // the difference is that this value is not divided by the consecutive
   // pipeline as for gl_Position, hence giving us what we have looked for
   gl_TexCoord[1] = gl_ModelViewProjectionMatrix * gl_Vertex;
}
