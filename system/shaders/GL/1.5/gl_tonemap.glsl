#if (defined(KODI_TONE_MAPPING_REINHARD) || defined(KODI_TONE_MAPPING_ACES) || defined(KODI_TONE_MAPPING_HABLE))
const float ST2084_m1 = 2610.0 / (4096.0 * 4.0);
const float ST2084_m2 = (2523.0 / 4096.0) * 128.0;
const float ST2084_c1 = 3424.0 / 4096.0;
const float ST2084_c2 = (2413.0 / 4096.0) * 32.0;
const float ST2084_c3 = (2392.0 / 4096.0) * 32.0;
#endif

#if defined(KODI_TONE_MAPPING_REINHARD)
float reinhard(float x)
{
  return x * (1.0 + x / (m_toneP1 * m_toneP1)) / (1.0 + x);
}
#endif

#if defined(KODI_TONE_MAPPING_ACES)
vec3 aces(vec3 x)
{
  float A = 2.51;
  float B = 0.03;
  float C = 2.43;
  float D = 0.59;
  float E = 0.14;
  return (x * (A * x + B)) / (x * (C * x + D) + E);
}
#endif

#if defined(KODI_TONE_MAPPING_HABLE)
vec3 hable(vec3 x)
{
  float A = 0.15;
  float B = 0.5;
  float C = 0.1;
  float D = 0.2;
  float E = 0.02;
  float F = 0.3;
  return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}
#endif

#if (defined(KODI_TONE_MAPPING_ACES) || defined(KODI_TONE_MAPPING_HABLE))
vec3 inversePQ(vec3 x)
{
  x = pow(max(x, 0.0), vec3(1.0 / ST2084_m2));
  x = max(x - ST2084_c1, 0.0) / (ST2084_c2 - ST2084_c3 * x);
  x = pow(x, vec3(1.0 / ST2084_m1));
  return x;
}
#endif

