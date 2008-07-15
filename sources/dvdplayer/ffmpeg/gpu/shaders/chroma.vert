uniform int tex_width, tex_height;

void main(void)
{
  // Handling the scaling to normalized coords in
  // the shader.
  vec4 scale = vec4(tex_width, tex_height, 1.0, 1.0);
  vec4 mv_scale = vec4(8.0, 8.0, 1.0, 0.0);

  vec4 push = vec4(0.0, 0.0, 4.0, 0.0);

  // Store the full portion in TexCoord 0, and the fractional part in TexCoord1
  // Store the width of a texel in TexCoord2
  gl_TexCoord[0] = (floor(gl_Vertex/2.0) + (floor(gl_MultiTexCoord1/mv_scale)))/scale;
  gl_TexCoord[1] = fract(gl_MultiTexCoord1/mv_scale)*8.0;
  gl_TexCoord[2].xy = vec2(1.0/tex_width, 1.0/tex_height);
  gl_Position = gl_ModelViewProjectionMatrix * (gl_Vertex+push);
}
