/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

///////////////////////////////////////////////////////////////////////
// Interpolators.h
// A couple of tweening classes implemented in C++.
// ref: http://www.robertpenner.com/easing/
//
// Author: d4rk <d4rk@xbmc.org>
///////////////////////////////////////////////////////////////////////

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

enum class EASE
{
  IN,
  OUT,
  INOUT
};

class Interpolator
{
public:
  explicit Interpolator(EASE easeType = EASE::OUT) : m_easeType(easeType) {}
  virtual ~Interpolator() = default;

  void SetEasing(EASE type) { m_easeType = type; }
  virtual float Tween(float time, float start, float change, float duration)
  {
    return Interpolate(time / duration) * change + start;
  }
  /*!
   \brief Maps an input [0,1] to a interpolation function
   */
  virtual float Interpolate(float phase) { return 0.0f; };
  virtual bool HasResumePoint() const { return m_easeType == EASE::INOUT; }
  /*!
   \brief Returns minimum steps required for piecewise linear interpolation
   */
  virtual unsigned int GetMinSteps() const { return 32; }

protected:
  EASE m_easeType;
};

class LinearInterpolator : public Interpolator
{
public:
  float Interpolate(float phase) override { return phase; }
  bool HasResumePoint() const override { return false; }
  unsigned int GetMinSteps() const override { return 2; }
};

class QuadInterpolator : public Interpolator
{
public:
  explicit QuadInterpolator(float a = 1.0f) : _a(a) {}

  float Interpolate(float phase) override
  {
    switch (m_easeType)
    {
      case EASE::IN:
        return phase * (_a * phase + 1 - _a);

      default:
      case EASE::OUT:
        return -1.0f * phase * (_a * phase - 1 - _a);

      case EASE::INOUT:
        phase *= 2;
        if (phase < 1)
          return 0.5f * phase * (_a * phase + 1 - _a);
        phase--;
        return -0.5f * (phase * (_a * phase - 1 - _a) - 1);
    }
  }
  unsigned int GetMinSteps() const override { return _a <= 0.5f ? 8 : 16; }

private:
  float _a;
};

class CubicInterpolator : public Interpolator
{
public:
  float Interpolate(float phase) override
  {
    switch (m_easeType)
    {
      case EASE::IN:
        return phase * phase * phase;

      default:
      case EASE::OUT:
        phase--;
        return (phase * phase * phase + 1);

      case EASE::INOUT:
        phase *= 2;
        if (phase < 1)
          return 0.5f * phase * phase * phase;
        phase -= 2;
        return 0.5f * (phase * phase * phase + 2);
    }
  }
  unsigned int GetMinSteps() const override { return 16; }
};

class CircleInterpolator : public Interpolator
{
public:
  float Interpolate(float phase) override
  {
    switch (m_easeType)
    {
      case EASE::IN:
        return -1.0f * (sqrt(1 - phase * phase) - 1);

      default:
      case EASE::OUT:
        phase--;
        return sqrt(1 - phase * phase);

      case EASE::INOUT:
        phase *= 2;
        if (phase < 1)
          return -0.5f * (sqrt(1 - phase * phase) - 1);
        phase -= 2;
        return 0.5f * (sqrt(1 - phase * phase) + 1);
    }
  }
};

class BackInterpolator : public Interpolator
{
public:
  explicit BackInterpolator(float s = 1.70158) : _s(s) {}

  float Interpolate(float phase) override
  {
    float s = _s;
    switch (m_easeType)
    {
      case EASE::IN:
        return phase * phase * ((s + 1) * phase - s);

      default:
      case EASE::OUT:
        phase--;
        return phase * phase * ((s + 1) * phase + s) + 1;

      case EASE::INOUT:
        phase *= 2;
        s *= (1.525f);
        if ((phase) < 1)
          return 0.5f * (phase * phase * ((s + 1) * phase - s));
        phase -= 2;
        return 0.5f * (phase * phase * ((s + 1) * phase + s) + 2);
    }
  }

private:
  float _s;
};

class SineInterpolator : public Interpolator
{
public:
  float Interpolate(float phase) override
  {
    switch (m_easeType)
    {
      case EASE::IN:
        return 1 - cos(phase * static_cast<float>(M_PI) / 2.0f);

      default:
      case EASE::OUT:
        return sin(phase * static_cast<float>(M_PI) / 2.0f);

      case EASE::INOUT:
        return 0.5f * (1 - cos(static_cast<float>(M_PI) * phase));
    }
  }
  unsigned int GetMinSteps() const override { return 8; }
};

class BounceInpolator : public Interpolator
{
public:
  float Tween(float time, float start, float change, float duration) override
  {
    switch (m_easeType)
    {
      case EASE::IN:
        return (change - easeOut(duration - time, 0, change, duration)) + start;

      default:
      case EASE::OUT:
        return easeOut(time, start, change, duration);

      case EASE::INOUT:
        if (time < duration / 2)
          return (change - easeOut(duration - (time * 2), 0, change, duration) + start) * .5f +
                 start;
        else
          return (easeOut(time * 2 - duration, 0, change, duration) * .5f + change * .5f) + start;
    }
  }

protected:
  static float easeOut(float time, float start, float change, float duration)
  {
    time /= duration;
    if (time < (1 / 2.75f))
    {
      return change * (7.5625f * time * time) + start;
    }
    else if (time < (2 / 2.75f))
    {
      time -= (1.5f / 2.75f);
      return change * (7.5625f * time * time + .75f) + start;
    }
    else if (time < (2.5f / 2.75f))
    {
      time -= (2.25f / 2.75f);
      return change * (7.5625f * time * time + .9375f) + start;
    }
    else
    {
      time -= (2.625f / 2.75f);
      return change * (7.5625f * time * time + .984375f) + start;
    }
  }
};

class ElasticInpolator : public Interpolator
{
public:
  ElasticInpolator(float a = 0.0, float p = 0.0) : _a(a), _p(p) {}

  float Tween(float time, float start, float change, float duration) override
  {
    switch (m_easeType)
    {
      case EASE::IN:
        return easeIn(time, start, change, duration);

      default:
      case EASE::OUT:
        return easeOut(time, start, change, duration);

      case EASE::INOUT:
        return easeInOut(time, start, change, duration);
    }
  }

protected:
  float _a;
  float _p;

  float easeIn(float time, float start, float change, float duration) const
  {
    float s = 0;
    float a = _a;
    float p = _p;

    if (time == 0)
      return start;
    time /= duration;
    if (time == 1)
      return start + change;
    if (!p)
      p = duration * .3f;
    if (!a || a < fabs(change))
    {
      a = change;
      s = p / 4.0f;
    }
    else
    {
      s = p / (2 * static_cast<float>(M_PI)) * asin(change / a);
    }
    time--;
    return -(a * pow(2.0f, 10 * time) *
             sin((time * duration - s) * (2 * static_cast<float>(M_PI)) / p)) +
           start;
  }

  float easeOut(float time, float start, float change, float duration) const
  {
    float s = 0;
    float a = _a;
    float p = _p;

    if (time == 0)
      return start;
    time /= duration;
    if (time == 1)
      return start + change;
    if (!p)
      p = duration * .3f;
    if (!a || a < fabs(change))
    {
      a = change;
      s = p / 4.0f;
    }
    else
    {
      s = p / (2 * static_cast<float>(M_PI)) * asin(change / a);
    }
    return (a * pow(2.0f, -10 * time) *
            sin((time * duration - s) * (2 * static_cast<float>(M_PI)) / p)) +
           change + start;
  }

  float easeInOut(float time, float start, float change, float duration) const
  {
    float s = 0;
    float a = _a;
    float p = _p;

    if (time == 0)
      return start;
    time /= duration / 2;
    if (time == 2)
      return start + change;
    if (!p)
      p = duration * .3f * 1.5f;
    if (!a || a < fabs(change))
    {
      a = change;
      s = p / 4.0f;
    }
    else
    {
      s = p / (2 * static_cast<float>(M_PI)) * asin(change / a);
    }

    if (time < 1)
    {
      time--;
      return -.5f * (a * pow(2.0f, 10 * (time)) *
                     sin((time * duration - s) * (2 * static_cast<float>(M_PI)) / p)) +
             start;
    }
    time--;
    return a * pow(2.0f, -10 * (time)) *
               sin((time * duration - s) * (2 * static_cast<float>(M_PI)) / p) * .5f +
           change + start;
  }
};
