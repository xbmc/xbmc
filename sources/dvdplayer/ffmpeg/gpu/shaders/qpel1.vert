uniform int tex_width, tex_height;

void main(void)
{
  // Handling the scaling to normalized coords in
  // the shader.
  vec4 scale = vec4(tex_width, tex_height, 1.0, 1.0);
  vec4 mv_scale = vec4(4.0, 4.0, 1.0, 0.0);

  vec4 push = vec4(0.0, 0.0, 4.0, 0.0);
  float mv_x = fract(gl_MultiTexCoord0.x/4.0);
  float mv_y = fract(gl_MultiTexCoord0.y/4.0);
  if( (mv_x == 0.0) || (mv_y == 0.0))
  {
    if( (mv_x == 0.25) || (mv_x == 0.75) || (mv_y == 0.25) || (mv_y == 0.75))
    {
      push.z = 0.0;
    }
  }

  // Store the full portion in TexCoord 0, and the fractional part in TexCoord1
  // Store the width of a texel in TexCoord2
  gl_TexCoord[0] = (gl_Vertex + (floor(gl_MultiTexCoord0/mv_scale)))/scale;
  gl_TexCoord[1] = fract(gl_MultiTexCoord0/mv_scale)/scale;
  gl_TexCoord[2].xy = vec2(1.0/float(tex_width), 1.0/float(tex_height));
  gl_Position = gl_ModelViewProjectionMatrix * (gl_Vertex+push);
}
