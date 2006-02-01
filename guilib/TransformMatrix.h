#pragma once

class TransformMatrix
{
public:
  TransformMatrix()
  {
    m[0][0] = 1.0f; m[0][1] = m[0][2] = 0;
    m[1][0] = m[1][2] = 0; m[1][1] = 1.0f;
    alpha = 1.0f;
  };
  static TransformMatrix CreateScaler(float scaleX, float scaleY)
  {
    TransformMatrix scaler;
    scaler.m[0][0] = scaleX;
    scaler.m[1][1] = scaleY;
    return scaler;
  }
  static TransformMatrix CreateTranslation(float transX, float transY)
  {
    TransformMatrix translation;
    translation.m[0][2] = transX;
    translation.m[1][2] = transY;
    return translation;
  }
  static TransformMatrix CreateRotation(float angle)
  {
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

  // assignment operator
  const TransformMatrix &operator =(const TransformMatrix &right)
  {
    if (this != &right)
    {
      memcpy(m, right.m, 6*sizeof(float));
      alpha = right.alpha;
    }
    return *this;
  }

  // multiplication operators
  const TransformMatrix &operator *=(const TransformMatrix &right)
  {
    float t00 = m[0][0] * right.m[0][0] + m[0][1] * right.m[1][0];
    float t01 = m[0][0] * right.m[0][1] + m[0][1] * right.m[1][1];
    m[0][2] = m[0][0] * right.m[0][2] + m[0][1] * right.m[1][2] + m[0][2];
    m[0][0] = t00; m[0][1] = t01;
    t00 = m[1][0] * right.m[0][0] + m[1][1] * right.m[1][0];
    t01 = m[1][0] * right.m[0][1] + m[1][1] * right.m[1][1];
    m[1][2] = m[1][0] * right.m[0][2] + m[1][1] * right.m[1][2] + m[1][2];
    m[1][0] = t00; m[1][1] = t01;
    alpha *= right.alpha;
    return *this;
  }

  TransformMatrix operator *(const TransformMatrix &right)
  {
    TransformMatrix result;
    result.m[0][0] = m[0][0] * right.m[0][0] + m[0][1] * right.m[1][0];
    result.m[0][1] = m[0][0] * right.m[0][1] + m[0][1] * right.m[1][1];
    result.m[0][2] = m[0][0] * right.m[0][2] + m[0][1] * right.m[1][2] + m[0][2];
    result.m[1][0] = m[1][0] * right.m[0][0] + m[1][1] * right.m[1][0];
    result.m[1][1] = m[1][0] * right.m[0][1] + m[1][1] * right.m[1][1];
    result.m[1][2] = m[1][0] * right.m[0][2] + m[1][1] * right.m[1][2] + m[1][2];
    result.alpha = alpha * right.alpha;
    return result;
  }

  inline void TransformPosition(float &x, float &y) const
  {
    float newX = m[0][0] * x + m[0][1] * y + m[0][2];
    y = m[1][0] * x + m[1][1] * y + m[1][2];
    x = newX;
  }

  inline float TransformXCoord(float x, float y) const
  {
    return m[0][0] * x + m[0][1] * y + m[0][2];
  }

  inline float TransformYCoord(float x, float y) const
  {
    return m[1][0] * x + m[1][1] * y + m[1][2];
  }

  inline DWORD TransformAlpha(DWORD colour) const
  {
    return (DWORD)(colour * alpha);
  }

private:
  float m[2][3];
  float alpha;
};