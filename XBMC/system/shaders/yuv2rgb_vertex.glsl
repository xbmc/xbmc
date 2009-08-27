varying vec2 m_cordY;
varying vec2 m_cordU;
varying vec2 m_cordV;

void main()
{
#if(XBMC_texture_rectangle_hack)
  m_cordY = vec2(gl_TextureMatrix[0] * gl_MultiTexCoord0 / 2);
  m_cordU = vec2(gl_TextureMatrix[1] * gl_MultiTexCoord1 * 2);
  m_cordV = vec2(gl_TextureMatrix[2] * gl_MultiTexCoord2);
#else
  m_cordY = vec2(gl_TextureMatrix[0] * gl_MultiTexCoord0);
  m_cordU = vec2(gl_TextureMatrix[1] * gl_MultiTexCoord1);
  m_cordV = vec2(gl_TextureMatrix[2] * gl_MultiTexCoord2);
#endif
  gl_Position = ftransform();
  gl_FrontColor = gl_Color;
}
