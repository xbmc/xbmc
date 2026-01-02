#version 100
precision highp float;

uniform sampler2D m_samp0;

// Viewport in framebuffer pixels: x, y, w, h (bottom-left origin)
uniform vec4  m_viewport;

// Rect in framebuffer pixels, bottom-left origin: x1,y1,x2,y2
uniform vec4  m_maskRect;
uniform vec4  m_radii;    // corner radii in framebuffer pixels: tl,tr,br,bl
uniform float m_aaWidth;  // AA width in framebuffer pixels (e.g. 1.0)

float sdRoundRect(vec2 p, vec2 b, float r)
{
  vec2 q = abs(p) - b;
  return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main()
{
  vec2 p = gl_FragCoord.xy;

  // Local point relative to rect center (bottom-left origin in gl_FragCoord).
  vec2 center   = 0.5 * (m_maskRect.xy + m_maskRect.zw);
  vec2 halfSize = 0.5 * (m_maskRect.zw - m_maskRect.xy);
  vec2 d = p - center;

  // Pick the radius for the current quadrant (TL,TR,BR,BL).
  float rSel = (d.x < 0.0)
                 ? ((d.y < 0.0) ? m_radii.w : m_radii.x)
                 : ((d.y < 0.0) ? m_radii.z : m_radii.y);

  float r = clamp(rSel, 0.0, min(halfSize.x, halfSize.y));
  vec2  b = halfSize - vec2(r);


  // Sample full-screen offscreen buffer using framebuffer-relative UVs.
  vec2 vpSize = max(m_viewport.zw, vec2(1.0, 1.0));
  vec2 uv = (p - m_viewport.xy) / vpSize;
  uv = clamp(uv, vec2(0.0, 0.0), vec2(1.0, 1.0));
  vec4 src = texture2D(m_samp0, uv);

  float dist = sdRoundRect(d, b, r);

  float aa = max(m_aaWidth, 0.0001);
  float cov = 1.0 - smoothstep(0.0, aa, dist); // inside=1, outside=0

  // Apply coverage to BOTH rgb and alpha (straight alpha pipeline).
  gl_FragColor = vec4(src.rgb * cov, src.a * cov);
}
