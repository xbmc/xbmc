#define TEXWIDTH 512.0
#define TEXHEIGHT 2048.0

void main(void)
{
  vec4 scale = vec4(TEXWIDTH*4, TEXHEIGHT, 1.0, 0.0);
  vec4 mv_scale = vec4(4.0, 4.0, 1.0, 0.0);
  vec4 push = vec4(0.0, 0.0, 4.0, 0.0);
  float mv_x = frac(gl_MultiTexCoord0.x/4.0);
  float mv_y = frac(gl_MultiTexCoord0.y/4.0);

  if( ((mv_x == 0.5) && (mv_y == 0.0)) || (mv_x == 0.0) && (mv_y == 0.5))
  {
      push.z = 0.0;
  }

  //Store the full portion in texCoord 0, fractional part in TexCoord1
  gl_TexCoord[0] = (gl_Vertex +(floor(gl_MultiTexCoord0/mv_scale)))/scale;
  gl_TexCoord[1] = -sign(fract(gl_MultiTexCoord0/4.0));
  gl_Position = gl_ModelViewProjectionMatrix * (gl_Vertex+push);

}
    
