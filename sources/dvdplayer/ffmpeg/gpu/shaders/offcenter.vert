//gl_Color contains motion vectors


void main(void)
{

  vec4 push = vec4(0.0, 0.0, 0.0, 4.0);

  if( (mod(gl_Color.r, 4.0) || mod(gl_Color.g, 4.0)))
  {//only work on off center pels
    push.z = 0;
  }

  //Accomplish the fullpel part of the interpolation
  gl_TexCoord[0] = gl_MultiTexcoord0 + floor(gl_Color/4);
  gl_Position = gl_ModelViewProjectionMatrix * (gl_Vertex+push);
}





