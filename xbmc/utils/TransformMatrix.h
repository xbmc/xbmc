/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/ColorUtils.h"

#include <algorithm>
#include <math.h>
#include <memory>
#include <string.h>

#ifdef __GNUC__
// under gcc, inline will only take place if optimizations are applied (-O). this will force inline even with optimizations.
#define XBMC_FORCE_INLINE __attribute__((always_inline))
#else
#define XBMC_FORCE_INLINE
#endif

class TransformMatrix
{
public:
  TransformMatrix()
  {
    Reset();
  };
  void Reset()
  {
    m[0][0] = 1.0f; m[0][1] = m[0][2] = m[0][3] = 0.0f;
    m[1][0] = m[1][2] = m[1][3] = 0.0f; m[1][1] = 1.0f;
    m[2][0] = m[2][1] = m[2][3] = 0.0f; m[2][2] = 1.0f;
    alpha = red = green = blue = 1.0f;
    identity = true;
    depth = 0;
  };
  static TransformMatrix CreateTranslation(float transX, float transY, float transZ = 0)
  {
    TransformMatrix translation;
    translation.SetTranslation(transX, transY, transZ);
    return translation;
  }
  void SetTranslation(float transX, float transY, float transZ)
  {
    m[0][1] = m[0][2] = 0.0f; m[0][0] = 1.0f; m[0][3] = transX;
    m[1][0] = m[1][2] = 0.0f; m[1][1] = 1.0f; m[1][3] = transY;
    m[2][0] = m[2][1] = 0.0f; m[2][2] = 1.0f; m[2][3] = transZ;
    alpha = red = green = blue = 1.0f;
    identity = (transX == 0 && transY == 0 && transZ == 0);
    depth = 0;
  }
  static TransformMatrix CreateScaler(float scaleX, float scaleY, float scaleZ = 1.0f)
  {
    TransformMatrix scaler;
    scaler.m[0][0] = scaleX;
    scaler.m[1][1] = scaleY;
    scaler.m[2][2] = scaleZ;
    scaler.identity = (scaleX == 1 && scaleY == 1 && scaleZ == 1);
    return scaler;
  };
  void SetScaler(float scaleX, float scaleY, float centerX, float centerY)
  {
    // Trans(centerX,centerY,centerZ)*Scale(scaleX,scaleY,scaleZ)*Trans(-centerX,-centerY,-centerZ)
    float centerZ = 0.0f, scaleZ = 1.0f;
    m[0][0] = scaleX;  m[0][1] = 0.0f;    m[0][2] = 0.0f;    m[0][3] = centerX*(1-scaleX);
    m[1][0] = 0.0f;    m[1][1] = scaleY;  m[1][2] = 0.0f;    m[1][3] = centerY*(1-scaleY);
    m[2][0] = 0.0f;    m[2][1] = 0.0f;    m[2][2] = scaleZ;  m[2][3] = centerZ*(1-scaleZ);
    alpha = red = green = blue = 1.0f;
    identity = (scaleX == 1 && scaleY == 1);
    depth = 0;
  };
  void SetXRotation(float angle, float y, float z, float ar = 1.0f)
  { // angle about the X axis, centered at y,z where our coordinate system has aspect ratio ar.
    // Trans(0,y,z)*Scale(1,1/ar,1)*RotateX(angle)*Scale(ar,1,1)*Trans(0,-y,-z);
    float c = cos(angle); float s = sin(angle);
    m[0][0] = ar;    m[0][1] = 0.0f;  m[0][2] = 0.0f;   m[0][3] = 0.0f;
    m[1][0] = 0.0f;  m[1][1] = c/ar;  m[1][2] = -s/ar;  m[1][3] = (-y*c+s*z)/ar + y;
    m[2][0] = 0.0f;  m[2][1] = s;     m[2][2] = c;      m[2][3] = (-y*s-c*z) + z;
    alpha = red = green = blue = 1.0f;
    identity = (angle == 0);
    depth = 0;
  }
  void SetYRotation(float angle, float x, float z, float ar = 1.0f)
  { // angle about the Y axis, centered at x,z where our coordinate system has aspect ratio ar.
    // Trans(x,0,z)*Scale(1/ar,1,1)*RotateY(angle)*Scale(ar,1,1)*Trans(-x,0,-z);
    float c = cos(angle); float s = sin(angle);
    m[0][0] = c;     m[0][1] = 0.0f;  m[0][2] = -s/ar;  m[0][3] = -x*c + s*z/ar + x;
    m[1][0] = 0.0f;  m[1][1] = 1.0f;  m[1][2] = 0.0f;   m[1][3] = 0.0f;
    m[2][0] = ar*s;  m[2][1] = 0.0f;  m[2][2] = c;      m[2][3] = -ar*x*s - c*z + z;
    alpha = red = green = blue = 1.0f;
    identity = (angle == 0);
    depth = 0;
  }
  static TransformMatrix CreateZRotation(float angle, float x, float y, float ar = 1.0f)
  { // angle about the Z axis, centered at x,y where our coordinate system has aspect ratio ar.
    // Trans(x,y,0)*Scale(1/ar,1,1)*RotateZ(angle)*Scale(ar,1,1)*Trans(-x,-y,0)
    TransformMatrix rot;
    rot.SetZRotation(angle, x, y, ar);
    return rot;
  }
  void SetZRotation(float angle, float x, float y, float ar = 1.0f)
  { // angle about the Z axis, centered at x,y where our coordinate system has aspect ratio ar.
    // Trans(x,y,0)*Scale(1/ar,1,1)*RotateZ(angle)*Scale(ar,1,1)*Trans(-x,-y,0)
    float c = cos(angle); float s = sin(angle);
    m[0][0] = c;     m[0][1] = -s/ar;  m[0][2] = 0.0f;  m[0][3] = -x*c + s*y/ar + x;
    m[1][0] = s*ar;  m[1][1] = c;      m[1][2] = 0.0f;  m[1][3] = -ar*x*s - c*y + y;
    m[2][0] = 0.0f;  m[2][1] = 0.0f;   m[2][2] = 1.0f;  m[2][3] = 0.0f;
    alpha = red = green = blue = 1.0f;
    identity = (angle == 0);
    depth = 0;
  }
  static TransformMatrix CreateFader(float a)
  {
    TransformMatrix fader;
    fader.SetFader(a);
    return fader;
  }
  static TransformMatrix CreateFader(float a, float r, float g, float b)
  {
    TransformMatrix fader;
    fader.SetFader(a, r, g, b);
    return fader;
  }
  void SetFader(float a)
  {
    m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f; m[0][3] = 0.0f;
    m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f; m[1][3] = 0.0f;
    m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f; m[2][3] = 0.0f;
    alpha = a;
    red = green = blue = 1.0f;
    identity = (a == 1.0f);
    depth = 0;
  }

  void SetFader(float a, float r, float g, float b)
  {
    m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f; m[0][3] = 0.0f;
    m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f; m[1][3] = 0.0f;
    m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f; m[2][3] = 0.0f;
    alpha = a;
    red = r;
    green = g;
    blue = b;
    identity = ((a == 1.0f) && (r == 1.0f) && (g == 1.0f) && (b == 1.0f));
    depth = 0;
  }

  // multiplication operators
  const TransformMatrix &operator *=(const TransformMatrix &right)
  {
    if (right.identity)
      return *this;
    if (identity)
    {
      *this = right;
      return *this;
    }
    float t00 = m[0][0] * right.m[0][0] + m[0][1] * right.m[1][0] + m[0][2] * right.m[2][0];
    float t01 = m[0][0] * right.m[0][1] + m[0][1] * right.m[1][1] + m[0][2] * right.m[2][1];
    float t02 = m[0][0] * right.m[0][2] + m[0][1] * right.m[1][2] + m[0][2] * right.m[2][2];
    m[0][3] = m[0][0] * right.m[0][3] + m[0][1] * right.m[1][3] + m[0][2] * right.m[2][3] + m[0][3];
    m[0][0] = t00; m[0][1] = t01; m[0][2] = t02;
    t00 = m[1][0] * right.m[0][0] + m[1][1] * right.m[1][0] + m[1][2] * right.m[2][0];
    t01 = m[1][0] * right.m[0][1] + m[1][1] * right.m[1][1] + m[1][2] * right.m[2][1];
    t02 = m[1][0] * right.m[0][2] + m[1][1] * right.m[1][2] + m[1][2] * right.m[2][2];
    m[1][3] = m[1][0] * right.m[0][3] + m[1][1] * right.m[1][3] + m[1][2] * right.m[2][3] + m[1][3];
    m[1][0] = t00; m[1][1] = t01; m[1][2] = t02;
    t00 = m[2][0] * right.m[0][0] + m[2][1] * right.m[1][0] + m[2][2] * right.m[2][0];
    t01 = m[2][0] * right.m[0][1] + m[2][1] * right.m[1][1] + m[2][2] * right.m[2][1];
    t02 = m[2][0] * right.m[0][2] + m[2][1] * right.m[1][2] + m[2][2] * right.m[2][2];
    m[2][3] = m[2][0] * right.m[0][3] + m[2][1] * right.m[1][3] + m[2][2] * right.m[2][3] + m[2][3];
    m[2][0] = t00; m[2][1] = t01; m[2][2] = t02;
    alpha *= right.alpha;
    red *= right.red;
    green *= right.green;
    blue *= right.blue;
    identity = false;
    depth = std::max(depth, right.depth);
    return *this;
  }

  TransformMatrix operator *(const TransformMatrix &right) const
  {
    if (right.identity)
      return *this;
    if (identity)
      return right;
    TransformMatrix result;
    result.m[0][0] = m[0][0] * right.m[0][0] + m[0][1] * right.m[1][0] + m[0][2] * right.m[2][0];
    result.m[0][1] = m[0][0] * right.m[0][1] + m[0][1] * right.m[1][1] + m[0][2] * right.m[2][1];
    result.m[0][2] = m[0][0] * right.m[0][2] + m[0][1] * right.m[1][2] + m[0][2] * right.m[2][2];
    result.m[0][3] = m[0][0] * right.m[0][3] + m[0][1] * right.m[1][3] + m[0][2] * right.m[2][3] + m[0][3];
    result.m[1][0] = m[1][0] * right.m[0][0] + m[1][1] * right.m[1][0] + m[1][2] * right.m[2][0];
    result.m[1][1] = m[1][0] * right.m[0][1] + m[1][1] * right.m[1][1] + m[1][2] * right.m[2][1];
    result.m[1][2] = m[1][0] * right.m[0][2] + m[1][1] * right.m[1][2] + m[1][2] * right.m[2][2];
    result.m[1][3] = m[1][0] * right.m[0][3] + m[1][1] * right.m[1][3] + m[1][2] * right.m[2][3] + m[1][3];
    result.m[2][0] = m[2][0] * right.m[0][0] + m[2][1] * right.m[1][0] + m[2][2] * right.m[2][0];
    result.m[2][1] = m[2][0] * right.m[0][1] + m[2][1] * right.m[1][1] + m[2][2] * right.m[2][1];
    result.m[2][2] = m[2][0] * right.m[0][2] + m[2][1] * right.m[1][2] + m[2][2] * right.m[2][2];
    result.m[2][3] = m[2][0] * right.m[0][3] + m[2][1] * right.m[1][3] + m[2][2] * right.m[2][3] + m[2][3];
    result.alpha = alpha * right.alpha;
    result.red = red * right.red;
    result.green = green * right.green;
    result.blue = blue * right.blue;
    result.identity = false;
    result.depth = std::max(depth, right.depth);
    return result;
  }

  inline void TransformPosition(float &x, float &y, float &z) const XBMC_FORCE_INLINE
  {
    float newX = m[0][0] * x + m[0][1] * y + m[0][2] * z + m[0][3];
    float newY = m[1][0] * x + m[1][1] * y + m[1][2] * z + m[1][3];
    z = m[2][0] * x + m[2][1] * y + m[2][2] * z + m[2][3];
    y = newY;
    x = newX;
  }

  inline void TransformPositionUnscaled(float &x, float &y, float &z) const XBMC_FORCE_INLINE
  {
    float n;
    // calculate the norm of the transformed (but not translated) vectors involved
    n = sqrt(m[0][0]*m[0][0] + m[0][1]*m[0][1] + m[0][2]*m[0][2]);
    float newX = (m[0][0] * x + m[0][1] * y + m[0][2] * z)/n + m[0][3];
    n = sqrt(m[1][0]*m[1][0] + m[1][1]*m[1][1] + m[1][2]*m[1][2]);
    float newY = (m[1][0] * x + m[1][1] * y + m[1][2] * z)/n + m[1][3];
    n = sqrt(m[2][0]*m[2][0] + m[2][1]*m[2][1] + m[2][2]*m[2][2]);
    float newZ = (m[2][0] * x + m[2][1] * y + m[2][2] * z)/n + m[2][3];
    z = newZ;
    y = newY;
    x = newX;
  }

  inline void InverseTransformPosition(float &x, float &y) const XBMC_FORCE_INLINE
  { // used for mouse - no way to find z
    x -= m[0][3]; y -= m[1][3];
    float detM = m[0][0]*m[1][1] - m[0][1]*m[1][0];
    float newX = (m[1][1] * x - m[0][1] * y)/detM;
    y = (-m[1][0] * x + m[0][0] * y)/detM;
    x = newX;
  }

  inline float TransformXCoord(float x, float y, float z) const XBMC_FORCE_INLINE
  {
    return m[0][0] * x + m[0][1] * y + m[0][2] * z + m[0][3];
  }

  inline float TransformYCoord(float x, float y, float z) const XBMC_FORCE_INLINE
  {
    return m[1][0] * x + m[1][1] * y + m[1][2] * z + m[1][3];
  }

  inline float TransformZCoord(float x, float y, float z) const XBMC_FORCE_INLINE
  {
    return m[2][0] * x + m[2][1] * y + m[2][2] * z + m[2][3];
  }

  inline UTILS::COLOR::Color TransformAlpha(UTILS::COLOR::Color color) const XBMC_FORCE_INLINE
  {
    return static_cast<UTILS::COLOR::Color>(color * alpha);
  }

  inline UTILS::COLOR::Color TransformColor(UTILS::COLOR::Color color) const XBMC_FORCE_INLINE
  {
    UTILS::COLOR::Color a = static_cast<UTILS::COLOR::Color>(((color >> 24) & 0xff) * alpha);
    UTILS::COLOR::Color r = static_cast<UTILS::COLOR::Color>(((color >> 16) & 0xff) * red);
    UTILS::COLOR::Color g = static_cast<UTILS::COLOR::Color>(((color >> 8) & 0xff) * green);
    UTILS::COLOR::Color b = static_cast<UTILS::COLOR::Color>(((color)&0xff) * blue);
    if (a > 255)
      a = 255;
    if (r > 255)
      r = 255;
    if (g > 255)
      g = 255;
    if (b > 255)
      b = 255;

    return ((a << 24) & 0xff000000) | ((r << 16) & 0xff0000) | ((g << 8) & 0xff00) | (b & 0xff);
  }

  float m[3][4];
  float alpha;
  float red;
  float green;
  float blue;
  bool identity;
  uint32_t depth{0};
};

inline bool operator==(const TransformMatrix &a, const TransformMatrix &b)
{
  bool comparison =
      a.alpha == b.alpha && a.red == b.red && a.green == b.green && a.blue == b.blue &&
      a.depth == b.depth &&
      ((a.identity && b.identity) ||
       (!a.identity && !b.identity &&
        std::equal(&a.m[0][0], &a.m[0][0] + sizeof(a.m) / sizeof(a.m[0][0]), &b.m[0][0])));
  return comparison;
}

inline bool operator!=(const TransformMatrix &a, const TransformMatrix &b)
{
  return !operator==(a, b);
}
