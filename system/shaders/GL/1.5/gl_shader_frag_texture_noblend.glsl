#version 150

uniform sampler2D m_samp0;
in vec4 m_cord0;
out vec4 fragColor;

// SM_TEXTURE_NOBLEND shader
void main ()
{
  fragColor.rgba = vec4(texture(m_samp0, m_cord0.xy).rgba);
}
