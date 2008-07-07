#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect textureIn;

void main(void)
{
  gl_FragColor = texture2DRect(textureIn, gl_TexCoord[0].st)/255;
  //gl_FragColor = vec4(5.0, 0.0, 0.0, 0.0);
}
