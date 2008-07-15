//Basic vertex shader
uniform int tex_width, tex_height;
void main()
{    
  // vec4 scale = vec4(tex_width, tex_height, 1, 1);
  gl_TexCoord[0] = gl_Vertex;
  gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
}
