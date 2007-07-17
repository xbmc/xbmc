#ifndef __TWEEN_H__
#define __TWEEN_H__

///////////////////////////////////////////////////////////////////////
// Tween.h
// A couple of tweening classes implemented in C++.
// ref: http://www.robertpenner.com/easing/
//
// Author: d4rk <d4rk@xboxmediacenter.com>
///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
// Current list of classes:
//
// LinearTweener
// QuadTweener
// CubicTweener
// SineTweener
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
  Tweener(TweenerType tweenerType = EASE_OUT) { m_tweenerType = tweenerType; _ref=0; }
  virtual ~Tweener() {};
  
  void SetEasing(TweenerType type) { m_tweenerType = type; }
  virtual float Tween(float time, float start, float change, float duration)=0;
  void Free() { _ref--; if (_ref==0) delete this; }
  void IncRef() { _ref++; }
  
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
};


class QuadTweener : public Tweener
{
public:
  virtual float Tween(float time, float start, float change, float duration)
  {
    time /= duration;
    switch (m_tweenerType)
      {
      case EASE_IN:
	return change * time * time + start;
	break;

      case EASE_OUT:
	return (-change) * (time) * (time - 2) + start;
	break;

      case EASE_INOUT:
	if (time/2 < 1)
	  return change/2 * time * time + start;
	time--;
	return -change/2 * (time * (time - 2) - 1) + start;
	break;
      }
    return change * (time /= duration) * time + start;
  }
};


class CubicTweener : public Tweener
{
public:
  virtual float Tween(float time, float start, float change, float duration)
  {
    time /= duration;
    switch (m_tweenerType)
      {
      case EASE_IN:
	return change * time * time * time + start;
	break;

      case EASE_OUT:
	time--;
	return change * (time * time * time + 1) + start;
	break;

      case EASE_INOUT:
	if (time/2 < 1)
	  return change/2 * time * time * time + start;
	time-=2;
	return change/2 * (time * time * time + 2) + start;
	break;
      }
    return change * (time /= duration) * time + start;
  }
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
	return change * (1 - cos(time * M_PI / 2.0)) + start;
	break;

      case EASE_OUT:
	return change * sin(time * M_PI / 2.0) + start;
	break;

      case EASE_INOUT:
	return change/2 * (1 - cos(M_PI * time)) + start;
	break;
      }
    return change/2 * (1 - cos(M_PI * time)) + start;
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
	return change - easeOut(duration - time, 0, change, duration) + start;
	break;

      case EASE_OUT:
	return easeOut(time, start, change, duration);
	break;

      case EASE_INOUT:
	if (time < duration/2) 
	  return (change - easeOut (duration - (time * 2), 0, change, duration) + start) * .5 + start;
	else 
	  return easeOut (time * 2 - duration, 0, change, duration) * .5 + change * .5 + start;
	break;
      }

    return easeOut(time, start, change, duration);
  }

protected:
  float easeOut(float time, float start, float change, float duration)
  {
    time /= duration;
    if (time < (1/2.75)) {
      return  change * (7.5625 * time * time) + start;
    } else if (time < (2/2.75)) {
      time -= (1.5/2.75);
      return change * (7.5625 * time * time + .75) + start;
    } else if (time < (2.5/2.75)) {
      time -= (2.25/2.75);
      return change * (7.5625 * time * time + .9375) + start;
    } else {
      time -= (2.625/2.75);
      return change * (7.5625 * time * time + .984375) + start;
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
    return easeIn(time, start, change, duration);
  }

protected:
  float _a;
  float _p;

  float easeIn(float time, float start, float change, float duration)
  {
    float s=0;
    if (time==0) 
      return start;  
    time /= duration;
    if (time==1) 
	return start + change;  
    if (!_p) 
      _p=duration*.3;
    if (!_a || _a < (double)fabs(change)) 
    { 
      _a = change; 
      s = _p / 4.0; 
    }
    else
    {
      s = _p / (2 * M_PI) * asin (change / _a);
    }
    time--;
    return -(_a * (double)pow(2,10*time) * sin((time * duration - s) * (2 * M_PI) / _p )) + start;
  }

  float easeOut(float time, float start, float change, float duration)
  {
    float s=0;
    if (time==0) 
      return start;  
    time /= duration;
    if (time==1) 
	return start + change;  
    if (!_p) 
      _p=duration*.3;
    if (!_a || _a < (double)fabs(change)) 
    { 
      _a = change; 
      s = _p / 4.0; 
    }
    else
    {
      s = _p / (2 * M_PI) * asin (change / _a);
    }
    return -(_a * (double)pow(2,-10*time) * sin((time * duration - s) * (2 * M_PI) / _p )) + change + start;
  }

  float easeInOut(float time, float start, float change, float duration)
  {
    float s=0;
    if (time==0) 
      return start;  
    time /= duration;
    if (time/2==2) 
	return start + change;  
    if (!_p) 
      _p=duration*.3*1.5;
    if (!_a || _a < (double)fabs(change)) 
    { 
      _a = change; 
      s = _p / 4.0; 
    }
    else
    {
      s = _p / (2 * M_PI) * asin (change / _a);
    }

    time--;
    if (time < 1) 
    {
      return -.5 * (_a * (double)pow(2,10 * (time)) * sin((time * duration - s) * (2 * M_PI) / _p )) + start;
    }
    return _a * (double)pow(2,-10 * (time)) * sin((time * duration-s) * (2 * M_PI)/_p ) * .5 + change + start;
  }
};



#endif // __TWEEN_H__
