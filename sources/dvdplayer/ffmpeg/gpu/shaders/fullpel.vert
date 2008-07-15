uniform int tex_width, tex_height;

void main(void)
{

  // Handling the scaling to normalized coords in
  // the shader.
  vec4 scale = vec4(tex_width, tex_height, 1.0, 1.0);
  vec4 mv_scale = vec4(4.0, 4.0, 1.0, 0.0);

  // Push fractional mv's out of the camera frustum
  vec4 push = vec4(0,0,0,0);
  push.z  = (abs(mod(gl_MultiTexCoord0.x, 4.0))+abs(mod(gl_MultiTexCoord0.y, 4.0)))*2.0;

  // Move texture coords to the pixel referenced by the mv
  // to accomplish fullpel motion comp.
  gl_TexCoord[0] = (gl_Vertex +(floor(gl_MultiTexCoord0/mv_scale)))/scale;
  gl_Position = gl_ModelViewProjectionMatrix * (gl_Vertex+push);
}
