#version 300 es

#extension GL_OES_EGL_image_external_essl3 : require

precision mediump float;
uniform samplerExternalOES m_samp0;
in vec4 m_cord0;
uniform float m_brightness;
uniform float m_contrast;
out vec4 fragColor;

void main ()
{
  vec4 rgb;

  rgb = texture(m_samp0, m_cord0.xy);
  rgb *= m_contrast;
  rgb += m_brightness;

  fragColor = rgb;
}
