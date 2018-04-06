#version 300 es

precision mediump float;

uniform sampler2D img;
in vec2 cord;
out vec4 fragColor;

void main()
{
  fragColor = texture(img, cord);
}
