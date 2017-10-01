#version 150

uniform sampler2D img;
in vec2 m_cord;
out vec4 fragColor;

void main()
{
  fragColor = texture(img, m_cord);
}
