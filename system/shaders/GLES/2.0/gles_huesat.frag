#if defined(KODI_HUESAT_PQ) || defined(KODI_HUESAT_HLG)
// PQ needs the precision of highp
#ifdef GL_FRAGMENT_PRECISION_HIGH
#define HS_PREC highp
#else
#define HS_PREC mediump
#endif

uniform HS_PREC mat3 m_hsMat;
uniform HS_PREC vec3 m_hsCoefs;
uniform HS_PREC float m_hsPeak; // linear, 1.0 = PQ 10000 cd/m2 or HLG nominal peak
uniform HS_PREC vec2 m_hsRange;

#if defined(KODI_HUESAT_PQ)
const HS_PREC float HS_PQ_m1 = 2610.0 / (4096.0 * 4.0);
const HS_PREC float HS_PQ_m2 = (2523.0 / 4096.0) * 128.0;
const HS_PREC float HS_PQ_c1 = 3424.0 / 4096.0;
const HS_PREC float HS_PQ_c2 = (2413.0 / 4096.0) * 32.0;
const HS_PREC float HS_PQ_c3 = (2392.0 / 4096.0) * 32.0;

// PQ EOTF, input non-linear [0,1], output linear [0,1] where 1.0 = 10000 cd/m2
HS_PREC vec3 hsDecodePQ(HS_PREC vec3 x)
{
  x = pow(clamp(x, 0.0, 1.0), vec3(1.0 / HS_PQ_m2));
  x = max(x - HS_PQ_c1, 0.0) / (HS_PQ_c2 - HS_PQ_c3 * x);
  x = pow(x, vec3(1.0 / HS_PQ_m1));
  return x;
}

// PQ inverse EOTF, input linear [0,1] where 1.0 = 10000 cd/m2, output non-linear [0,1]
HS_PREC vec3 hsEncodePQ(HS_PREC vec3 x)
{
  x = pow(max(x, 0.0), vec3(HS_PQ_m1));
  x = (HS_PQ_c1 + HS_PQ_c2 * x) / (1.0 + HS_PQ_c3 * x);
  x = pow(x, vec3(HS_PQ_m2));
  return x;
}
#else
const HS_PREC float HS_HLG_a = 0.17883277;
const HS_PREC float HS_HLG_b = 0.28466892;
const HS_PREC float HS_HLG_c = 0.55991073;

HS_PREC float hsDecodeHLG1(HS_PREC float x)
{
  return (x <= 0.5) ? x * x / 3.0 : (exp((x - HS_HLG_c) / HS_HLG_a) + HS_HLG_b) / 12.0;
}

HS_PREC float hsEncodeHLG1(HS_PREC float x)
{
  x = max(x, 0.0);
  return (x <= 1.0 / 12.0) ? sqrt(3.0 * x)
                           : HS_HLG_a * log(max(12.0 * x - HS_HLG_b, 1e-6)) + HS_HLG_c;
}

// HLG inverse OETF, input non-linear [0,1], output linear [0,1]
HS_PREC vec3 hsDecodeHLG(HS_PREC vec3 x)
{
  x = clamp(x, 0.0, 1.0);
  return vec3(hsDecodeHLG1(x.r), hsDecodeHLG1(x.g), hsDecodeHLG1(x.b));
}

// HLG OETF, input linear [0,1], output non-linear [0,1]
HS_PREC vec3 hsEncodeHLG(HS_PREC vec3 x)
{
  return vec3(hsEncodeHLG1(x.r), hsEncodeHLG1(x.g), hsEncodeHLG1(x.b));
}
#endif

// Hue and saturation of HDR are adjusted in linear light at constant luminance.
// Chroma is then pulled towards grey until no channel leaves [0, peak], so
// clipping can't shift the hue.
HS_PREC vec3 hueSaturation(HS_PREC vec3 color)
{
  color = (color - m_hsRange.y) / m_hsRange.x;
#if defined(KODI_HUESAT_PQ)
  HS_PREC vec3 lin = hsDecodePQ(color);
#else
  HS_PREC vec3 lin = hsDecodeHLG(color);
#endif
  HS_PREC float luma = dot(lin, m_hsCoefs);
  HS_PREC vec3 chroma = m_hsMat * lin - luma;
  HS_PREC float top = max(m_hsPeak, max(lin.r, max(lin.g, lin.b)));

#ifdef GL_FRAGMENT_PRECISION_HIGH
  const HS_PREC float eps = 1e-6;
#else
  const HS_PREC float eps = 0.00006103515625;
#endif
  HS_PREC vec3 below = step(chroma, vec3(-eps));
  HS_PREC vec3 above = step(vec3(eps), chroma);
  HS_PREC vec3 negativeDen = max(-chroma, vec3(eps));
  HS_PREC vec3 positiveDen = max(chroma, vec3(eps));
  HS_PREC vec3 limit =
      1.0 + below * (min(vec3(luma), negativeDen) / negativeDen - 1.0)
          + above * (min(vec3(top - luma), positiveDen) / positiveDen - 1.0);
  lin = max(luma + clamp(min(limit.r, min(limit.g, limit.b)), 0.0, 1.0) * chroma, vec3(0.0));

#if defined(KODI_HUESAT_PQ)
  return hsEncodePQ(lin) * m_hsRange.x + m_hsRange.y;
#else
  return hsEncodeHLG(lin) * m_hsRange.x + m_hsRange.y;
#endif
}
#endif
