#define TEXWIDTH 512.0
#define TEXHEIGHT 2048.0

void main(void)
{
  // Push fractional mv's out of the camera frustum
  vec4 scale = vec4(TEXWIDTH*4, TEXHEIGHT, 1.0, 0.0);
  vec4 mv_scale = vec4(4.0, 4.0, 1.0, 0.0);
  vec4 push = vec4(0,0,0,0);
  push.z  = (abs(mod(gl_MultiTexCoord0.x, 4.0))+abs(mod(gl_MultiTexCoord0.y, 4.0)))*2.0;

  // Move texture coords to the pixel referenced by the mv
  // to accomplish fullpel motion comp.
  gl_TexCoord[0] = (gl_Vertex +(floor(gl_MultiTexCoord0/mv_scale)))/scale;
  gl_Position = gl_ModelViewProjectionMatrix * (gl_Vertex+push);
}
