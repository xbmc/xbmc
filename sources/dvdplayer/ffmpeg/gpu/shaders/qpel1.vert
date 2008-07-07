
void main(void)
{
  vec4 push = vec4(0.0, 0.0, 4.0, 0.0);
  float mv_x = frac(gl_Color.r/4);
  float mv_y = frac(gl_Color.g/4);


  if( (mv_x == 0) || (mv_y == 0))
  {
    if( (mv_x == 0.25) || (mv_x == 0.75) || (mv_y == 0.25) || (mv_y == 0.75))
    {
      push.z = 0.0;
    }
  }

  //Store the full portion in texCoord 0, fractional part in TexCoord1
  gl_TexCoord[0] = gl_MultiTexCoord0 + floor(gl_Color/4);
  gl_TexCoord[1] = fract(gl_Color/4);
  gl_Position = gl_ModelViewProjectionMatrix * (gl_Vertex+push);
}
