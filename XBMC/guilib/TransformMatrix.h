#pragma once

#include <math.h>
#include <memory>

class TransformMatrix
{
public:
  TransformMatrix()
  {
    Reset();
  };
  void Reset()
  {
    m[0][0] = 1.0f; m[0][1] = m[0][2] = m[0][3] = 0;
    m[1][0] = m[1][2] = m[1][3] = 0; m[1][1] = 1.0f;
    m[2][0] = m[2][1] = m[2][3] = 0; m[2][2] = 1.0f;
    alpha = 1.0f;
  };
  static TransformMatrix CreateScaler(float scaleX, float scaleY)
  {
    TransformMatrix scaler;
    scaler.m[0][0] = scaleX;
    scaler.m[1][1] = scaleY;
    return scaler;
  };
  void SetScaler(float scaleX, float scaleY)
  {
    m[0][0] = scaleX; m[0][1] = m[0][2] = m[0][3] = 0;
    m[1][1] = scaleY; m[1][0] = m[1][2] = m[1][3] = 0;
    m[2][2] = 1.0f;   m[2][0] = m[2][1] = m[2][3] = 0;
    alpha = 1.0f;
  };
  static TransformMatrix CreateTranslation(float transX, float transY, float transZ = 0)
  {
    TransformMatrix translation;
    translation.m[0][3] = transX;
    translation.m[1][3] = transY;
    translation.m[2][3] = transZ;
    return translation;
  }
  void SetTranslation(float transX, float transY, float transZ)
  {
    m[0][1] = m[0][2] = 0.0f; m[0][0] = 1.0f; m[0][3] = transX;
    m[1][0] = m[1][2] = 0.0f; m[1][1] = 1.0f; m[1][3] = transY;
    m[2][0] = m[2][1] = 0.0f; m[2][2] = 1.0f; m[2][3] = transZ;
    alpha = 1.0f;
  }
  static TransformMatrix CreateXRotation(float angle)
  { // angle about the X axis
    TransformMatrix rotation;
    rotation.m[1][1] = cos(angle);
    rotation.m[2][1] = sin(angle);
    rotation.m[1][2] = -rotation.m[1][0];
    rotation.m[2][2] = rotation.m[0][0];
    return rotation;
  }
  static TransformMatrix CreateYRotation(float angle)
  { // angle about the Y axis
    TransformMatrix rotation;
    rotation.m[0][0] = cos(angle);
    rotation.m[2][0] = sin(angle);
    rotation.m[0][2] = -rotation.m[1][0];
    rotation.m[2][2] = rotation.m[0][0];
    return rotation;
  }
  static TransformMatrix CreateZRotation(float angle)
  { // angle about the Z axis
    TransformMatrix rotation;
    rotation.m[0][0] = cos(angle);
    rotation.m[1][0] = sin(angle);
    rotation.m[0][1] = -rotation.m[1][0];
    rotation.m[1][1] = rotation.m[0][0];
    return rotation;
  }
  static TransformMatrix CreateFader(float a)
  {
    TransformMatrix fader;
    fader.alpha = a;
    return fader;
  }
  void SetFader(float a)
  {
    m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f; m[0][3] = 0.0f;
    m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f; m[1][3] = 0.0f;
    m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f; m[2][3] = 0.0f;
    alpha = a;
  }
  // assignment operator
  const TransformMatrix &operator =(const TransformMatrix &right)
  {
    if (this != &right)
    {
      memcpy(m, right.m, 12*sizeof(float));
      alpha = right.alpha;
    }
    return *this;
  }

  // multiplication operators
  const TransformMatrix &operator *=(const TransformMatrix &right)
  {
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
    return *this;
  }

  TransformMatrix operator *(const TransformMatrix &right) const
  {
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
    return result;
  }

  inline void TransformPosition(float &x, float &y, float &z) const
  {
    float newX = m[0][0] * x + m[0][1] * y + m[0][2] * z + m[0][3];
    float newY = m[1][0] * x + m[1][1] * y + m[1][2] * z + m[1][3];
    z = m[2][0] * x + m[2][1] * y + m[2][2] * z + m[2][3];
    y = newY;
    x = newX;
  }

  inline void TransformPositionUnscaled(float &x, float &y, float &z) const
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

  inline void InverseTransformPosition(float &x, float &y) const
  { // used for mouse - no way to find z
    x -= m[0][2]; y -= m[1][2];
    float detM = m[0][0]*m[1][1] - m[0][1]*m[1][0];
    float newX = (m[1][1] * x - m[0][1] * y)/detM;
    y = (-m[1][0] * x + m[0][0] * y)/detM;
    x = newX;
  }

  inline float TransformXCoord(float x, float y, float z) const
  {
    return m[0][0] * x + m[0][1] * y + m[0][2] * z + m[0][3];
  }

  inline float TransformYCoord(float x, float y, float z) const
  {
    return m[1][0] * x + m[1][1] * y + m[1][2] * z + m[1][3];
  }

  inline float TransformZCoord(float x, float y, float z) const
  {
    return m[2][0] * x + m[2][1] * y + m[2][2] * z + m[2][3];
  }

  inline unsigned int TransformAlpha(unsigned int colour) const
  {
    return (unsigned int)(colour * alpha);
  }

private:
  float m[3][4];
  float alpha;
};