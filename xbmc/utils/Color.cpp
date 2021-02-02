#include "Color.h"

UTILS::Color4f::Color4f(float r, float g, float b, float a) : m_r(r), m_g(g), m_b(b), m_a(a)
{
}

UTILS::Color4f::Color4f(UTILS::Color col)
{
  uint32_t r = ((col >> 16) & 0xFF);
  uint32_t g = ((col >> 8) & 0xFF);
  uint32_t b = ((col >> 0) & 0xFF);
  uint32_t a = ((col >> 24) & 0xFF);

  m_r = r / 255.0f;
  m_g = g / 255.0f;
  m_b = b / 255.0f;
  m_a = a / 255.0f;
}
