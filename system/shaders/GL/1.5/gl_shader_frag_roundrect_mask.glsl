#version 150

uniform sampler2D m_samp0;

// Viewport in framebuffer pixels: x, y, w, h (bottom-left origin)
uniform vec4  m_viewport;

// Rect in framebuffer pixels, bottom-left origin: x1,y1,x2,y2
uniform vec4  m_maskRect;
uniform vec4  m_radii;    // corner radii in framebuffer pixels: tl,tr,br,bl
uniform float m_aaWidth;  // AA width in framebuffer pixels

out vec4 fragColor;

float sdRoundRect(vec2 p, vec2 b, float r)
{
  vec2 q = abs(p) - b;
  return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - r;
}

void main()
{
  vec2 p = gl_FragCoord.xy;

  vec2 center   = 0.5 * (m_maskRect.xy + m_maskRect.zw);
  vec2 halfSize = 0.5 * (m_maskRect.zw - m_maskRect.xy);
  vec2 d = p - center;

  float rSel = (d.x < 0.0)
                 ? ((d.y < 0.0) ? m_radii.w : m_radii.x)
                 : ((d.y < 0.0) ? m_radii.z : m_radii.y);

  float r = clamp(rSel, 0.0, min(halfSize.x, halfSize.y));
  vec2  b = halfSize - vec2(r);

  vec2 vpSize = max(m_viewport.zw, vec2(1.0, 1.0));
  vec2 uv = (p - m_viewport.xy) / vpSize;
  uv = clamp(uv, vec2(0.0), vec2(1.0));

  vec4 src = texture(m_samp0, uv);

  float dist = sdRoundRect(d, b, r);

  float aa  = max(m_aaWidth, 0.0001);
  float cov = 1.0 - smoothstep(0.0, aa, dist);

  fragColor = vec4(src.rgb * cov, src.a * cov);
}
