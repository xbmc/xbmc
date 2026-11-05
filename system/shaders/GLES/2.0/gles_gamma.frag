#if defined(KODI_GAMMA)
#ifdef GL_FRAGMENT_PRECISION_HIGH
#define GAMMA_PREC highp
#else
#define GAMMA_PREC mediump
#endif

uniform GAMMA_PREC float m_gammaInv;
uniform GAMMA_PREC vec2 m_gammaRange;
#endif
