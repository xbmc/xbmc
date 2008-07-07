void main(void)
{
  gl_TexCoord[0] = gl_MultiTexCoord0 + gl_Color;
  //gl_TexCoord[0] = gl_MultiTexCoord0; 
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
