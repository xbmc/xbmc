#version 100

precision mediump float;

uniform sampler2D img;
varying vec2 cord;

void main()
{
  gl_FragColor = texture2D(img, cord);
}