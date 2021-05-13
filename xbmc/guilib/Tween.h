/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

///////////////////////////////////////////////////////////////////////
// Tween.h
// A couple of tweening classes implemented in C++.
// ref: http://www.robertpenner.com/easing/
//
// Author: d4rk <d4rk@xbmc.org>
///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
// Current list of classes:
//
// LinearTweener
// QuadTweener
// CubicTweener
// SineTweener
// CircleTweener
// BackTweener
// BounceTweener
// ElasticTweener
//
///////////////////////////////////////////////////////////////////////

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

enum TweenerType
{
  EASE_IN,
  EASE_OUT,
  EASE_INOUT
};


class Tweener
{
public:
  explicit Tweener(TweenerType tweenerType = EASE_OUT) { m_tweenerType = tweenerType; }
  virtual ~Tweener() = default;

  void SetEasing(TweenerType type) { m_tweenerType = type; }
  virtual float Tween(float time, float start, float change, float duration)=0;
  virtual bool HasResumePoint() const { return m_tweenerType == EASE_INOUT; }
protected:
  TweenerType m_tweenerType;
};


class LinearTweener : public Tweener
{
public:
  float Tween(float time, float start, float change, float duration) override
  {
    return change * time / duration + start;
  }
  bool HasResumePoint() const override { return false; }
};


class QuadTweener : public Tweener
{
public:
  explicit QuadTweener(float a = 1.0f) { _a=a; }
  float Tween(float time, float start, float change, float duration) override
  {
    switch (m_tweenerType)
      {
      case EASE_IN:
        time /= duration;
        return change * time * (_a * time + 1 - _a) + start;
        break;

      case EASE_OUT:
        time /= duration;
        return -change * time * (_a * time - 1 - _a) + start;
        break;

      case EASE_INOUT:
        time /= duration/2;
        if (time < 1)
          return (change) * time * (_a * time + 1 - _a) + start;
        time--;
        return (-change) * time * (_a * time - 1 - _a) + start;
        break;
      }
    return change * time * time + start;
  }
private:
  float _a;
};


class CubicTweener : public Tweener
{
public:
  float Tween(float time, float start, float change, float duration) override
  {
    switch (m_tweenerType)
      {
      case EASE_IN:
        time /= duration;
        return change * time * time * time + start;
        break;

      case EASE_OUT:
        time /= duration;
        time--;
        return change * (time * time * time + 1) + start;
        break;

      case EASE_INOUT:
        time /= duration/2;
        if (time < 1)
          return (change/2) * time * time * time + start;
        time-=2;
        return (change/2) * (time * time * time + 2) + start;
        break;
      }
    return change * time * time + start;
  }
};

class CircleTweener : public Tweener
{
public:
  float Tween(float time, float start, float change, float duration) override
  {
    switch (m_tweenerType)
      {
      case EASE_IN:
        time /= duration;
        return (-change) * (sqrt(1 - time * time) - 1) + start;
        break;

      case EASE_OUT:
        time /= duration;
        time--;
        return change * sqrt(1 - time * time) + start;
        break;

      case EASE_INOUT:
        time /= duration/2;
        if (time  < 1)
          return (-change/2) * (sqrt(1 - time * time) - 1) + start;
        time-=2;
        return change/2 * (sqrt(1 - time * time) + 1) + start;
        break;
      }
    return change * sqrt(1 - time * time) + start;
  }
};

class BackTweener : public Tweener
{
public:
  explicit BackTweener(float s=1.70158) { _s=s; }

  float Tween(float time, float start, float change, float duration) override
  {
    float s = _s;
    switch (m_tweenerType)
      {
      case EASE_IN:
        time /= duration;
        return change * time * time * ((s + 1) * time - s) + start;
        break;

      case EASE_OUT:
        time /= duration;
        time--;
        return change * (time * time * ((s + 1) * time + s) + 1) + start;
        break;

      case EASE_INOUT:
        time /= duration/2;
        s*=(1.525f);
        if ((time ) < 1)
        {
          return (change/2) * (time * time * ((s + 1) * time - s)) + start;
        }
        time-=2;
        return (change/2) * (time * time * ((s + 1) * time + s) + 2) + start;
        break;
      }
    return change * ((time-1) * time * ((s + 1) * time + s) + 1) + start;
  }
private:
  float _s;

};


class SineTweener : public Tweener
{
public:
  float Tween(float time, float start, float change, float duration) override
  {
    time /= duration;
    switch (m_tweenerType)
      {
      case EASE_IN:
        return change * (1 - cos(time * static_cast<float>(M_PI) / 2.0f)) + start;
        break;

      case EASE_OUT:
        return change * sin(time * static_cast<float>(M_PI) / 2.0f) + start;
        break;

      case EASE_INOUT:
        return change / 2 * (1 - cos(static_cast<float>(M_PI) * time)) + start;
        break;
      }
      return (change / 2) * (1 - cos(static_cast<float>(M_PI) * time)) + start;
  }
};


class BounceTweener : public Tweener
{
public:
  float Tween(float time, float start, float change, float duration) override
  {
    switch (m_tweenerType)
      {
      case EASE_IN:
        return (change - easeOut(duration - time, 0, change, duration)) + start;
        break;

      case EASE_OUT:
        return easeOut(time, start, change, duration);
        break;

      case EASE_INOUT:
        if (time < duration/2)
          return (change - easeOut (duration - (time * 2), 0, change, duration) + start) * .5f + start;
        else
          return (easeOut (time * 2 - duration, 0, change, duration) * .5f + change * .5f) + start;
        break;
      }

    return easeOut(time, start, change, duration);
  }
protected:
  static float easeOut(float time, float start, float change, float duration)
  {
    time /= duration;
    if (time < (1 / 2.75f))
    {
      return  change * (7.5625f * time * time) + start;
    }
    else if (time < (2 / 2.75f))
    {
      time -= (1.5f/2.75f);
      return change * (7.5625f * time * time + .75f) + start;
    }
    else if (time < (2.5f / 2.75f))
    {
      time -= (2.25f/2.75f);
      return change * (7.5625f * time * time + .9375f) + start;
    }
    else
    {
      time -= (2.625f/2.75f);
      return change * (7.5625f * time * time + .984375f) + start;
    }
  }
};


class ElasticTweener : public Tweener
{
public:
  ElasticTweener(float a=0.0, float p=0.0) { _a=a; _p=p; }

  float Tween(float time, float start, float change, float duration) override
  {
    switch (m_tweenerType)
      {
      case EASE_IN:
        return easeIn(time, start, change, duration);
        break;

      case EASE_OUT:
        return easeOut(time, start, change, duration);
        break;

      case EASE_INOUT:
        return easeInOut(time, start, change, duration);
        break;
      }
    return easeOut(time, start, change, duration);
  }
protected:
  float _a;
  float _p;

  float easeIn(float time, float start, float change, float duration) const
  {
    float s=0;
    float a=_a;
    float p=_p;

    if (time==0)
      return start;
    time /= duration;
    if (time==1)
        return start + change;
    if (!p)
      p=duration*.3f;
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
    float s=0;
    float a=_a;
    float p=_p;

    if (time==0)
      return start;
    time /= duration;
    if (time==1)
        return start + change;
    if (!p)
      p=duration*.3f;
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
    float s=0;
    float a=_a;
    float p=_p;

    if (time==0)
      return start;
    time /= duration/2;
    if (time==2)
        return start + change;
    if (!p)
      p=duration*.3f*1.5f;
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

