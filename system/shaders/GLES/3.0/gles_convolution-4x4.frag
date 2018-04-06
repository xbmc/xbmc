#version 300 es

precision highp float;

uniform sampler2D img;
uniform vec2 stepxy;
in vec2 cord;
uniform float m_alpha;
uniform sampler2D kernelTex;
out vec4 fragColor;

vec4 weight(float pos)
{
#if defined(HAS_FLOAT_TEXTURE)
  return texture(kernelTex, vec2(pos - 0.5));
#else
  return texture(kernelTex, vec2(pos - 0.5)) * 2.0 - 1.0;
#endif
}

vec3 pixel(float xpos, float ypos)
{
  return texture(img, vec2(xpos, ypos)).rgb;
}

vec3 line (float ypos, vec4 xpos, vec4 linetaps)
{
  return pixel(xpos.r, ypos) * linetaps.r +
         pixel(xpos.g, ypos) * linetaps.g +
         pixel(xpos.b, ypos) * linetaps.b +
         pixel(xpos.a, ypos) * linetaps.a;
}

void main()
{
  vec4 rgb;
  vec2 pos = cord + stepxy * 0.5;
  vec2 f = fract(pos / stepxy);

  vec4 linetaps = weight(1.0 - f.x);
  vec4 columntaps = weight(1.0 - f.y);

  // make sure all taps added together is exactly 1.0, otherwise some (very small) distortion can occur
  linetaps /= linetaps.r + linetaps.g + linetaps.b + linetaps.a;
  columntaps /= columntaps.r + columntaps.g + columntaps.b + columntaps.a;

  vec2 xystart = (-1.5 - f) * stepxy + pos;
  vec4 xpos = vec4(xystart.x, xystart.x + stepxy.x, xystart.x + stepxy.x * 2.0, xystart.x + stepxy.x * 3.0);

  rgb.rgb = line(xystart.y, xpos, linetaps) * columntaps.r +
            line(xystart.y + stepxy.y, xpos, linetaps) * columntaps.g +
            line(xystart.y + stepxy.y * 2.0, xpos, linetaps) * columntaps.b +
            line(xystart.y + stepxy.y * 3.0, xpos, linetaps) * columntaps.a;

  rgb.a = m_alpha;

  fragColor = rgb;
}

