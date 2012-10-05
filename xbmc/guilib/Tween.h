#ifndef __TWEEN_H__
#define __TWEEN_H__

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

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
#define M_PI 3.14159265358979323846f
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
  Tweener(TweenerType tweenerType = EASE_OUT) { m_tweenerType = tweenerType; _ref=1; }
  virtual ~Tweener() {};

  void SetEasing(TweenerType type) { m_tweenerType = type; }
  virtual float Tween(float time, float start, float change, float duration)=0;
  void Free() { _ref--; if (_ref==0) delete this; }
  void IncRef() { _ref++; }
  virtual bool HasResumePoint() const { return m_tweenerType == EASE_INOUT; }
protected:
  int _ref;
  TweenerType m_tweenerType;
};


class LinearTweener : public Tweener
{
public:
  virtual float Tween(float time, float start, float change, float duration)
  {
    return change * time / duration + start;
  }
  virtual bool HasResumePoint() const { return false; }
};


class QuadTweener : public Tweener
{
public:
  QuadTweener(float a = 1.0f) { _a=a; }
  virtual float Tween(float time, float start, float change, float duration)
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
  virtual float Tween(float time, float start, float change, float duration)
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
  virtual float Tween(float time, float start, float change, float duration)
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
  BackTweener(float s=1.70158) { _s=s; }

  virtual float Tween(float time, float start, float change, float duration)
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
  virtual float Tween(float time, float start, float change, float duration)
  {
    time /= duration;
    switch (m_tweenerType)
      {
      case EASE_IN:
        return change * (1 - cos(time * M_PI / 2.0f)) + start;
        break;

      case EASE_OUT:
        return change * sin(time * M_PI / 2.0f) + start;
        break;

      case EASE_INOUT:
        return change/2 * (1 - cos(M_PI * time)) + start;
        break;
      }
    return (change/2) * (1 - cos(M_PI * time)) + start;
  }
};


class BounceTweener : public Tweener
{
public:
  virtual float Tween(float time, float start, float change, float duration)
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
  float easeOut(float time, float start, float change, float duration)
  {
    time /= duration;
    if (time < (1/2.75)) {
      return  change * (7.5625f * time * time) + start;
    } else if (time < (2/2.75)) {
      time -= (1.5f/2.75f);
      return change * (7.5625f * time * time + .75f) + start;
    } else if (time < (2.5/2.75)) {
      time -= (2.25f/2.75f);
      return change * (7.5625f * time * time + .9375f) + start;
    } else {
      time -= (2.625f/2.75f);
      return change * (7.5625f * time * time + .984375f) + start;
    }
  }
};


class ElasticTweener : public Tweener
{
public:
  ElasticTweener(float a=0.0, float p=0.0) { _a=a; _p=p; }

  virtual float Tween(float time, float start, float change, float duration)
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

  float easeIn(float time, float start, float change, float duration)
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
      s = p / (2 * M_PI) * asin (change / a);
    }
    time--;
    return -(a * pow(2.0f, 10*time) * sin((time * duration - s) * (2 * M_PI) / p )) + start;
  }

  float easeOut(float time, float start, float change, float duration)
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
      s = p / (2 * M_PI) * asin (change / a);
    }
    return (a * pow(2.0f, -10*time) * sin((time * duration - s) * (2 * M_PI) / p )) + change + start;
  }

  float easeInOut(float time, float start, float change, float duration)
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
      s = p / (2 * M_PI) * asin (change / a);
    }

    if (time < 1)
    {
      time--;
      return -.5f * (a * pow(2.0f, 10 * (time)) * sin((time * duration - s) * (2 * M_PI) / p )) + start;
    }
    time--;
    return a * pow(2.0f, -10 * (time)) * sin((time * duration-s) * (2 * M_PI) / p ) * .5f + change + start;
  }
};



#endif // __TWEEN_H__
