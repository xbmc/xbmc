#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

enum ANIMATION_PROCESS { ANIM_PROCESS_NONE = 0, ANIM_PROCESS_NORMAL, ANIM_PROCESS_REVERSE };
enum ANIMATION_STATE { ANIM_STATE_NONE = 0, ANIM_STATE_DELAYED, ANIM_STATE_IN_PROCESS, ANIM_STATE_APPLIED };

// forward definitions

class TiXmlElement;
class Tweener;
class CGUIListItem;

#include "TransformMatrix.h"  // needed for the TransformMatrix member
#include "Geometry.h"         // for CPoint, CRect
#include "utils/StdString.h"
#include <memory>
#include "interfaces/info/InfoBool.h"

enum ANIMATION_TYPE
{
  ANIM_TYPE_UNFOCUS = -3,
  ANIM_TYPE_HIDDEN,
  ANIM_TYPE_WINDOW_CLOSE,
  ANIM_TYPE_NONE,
  ANIM_TYPE_WINDOW_OPEN,
  ANIM_TYPE_VISIBLE,
  ANIM_TYPE_FOCUS,
  ANIM_TYPE_CONDITIONAL       // for animations triggered by a condition change
};

class CAnimEffect
{
public:
  enum EFFECT_TYPE { EFFECT_TYPE_NONE = 0, EFFECT_TYPE_FADE, EFFECT_TYPE_SLIDE, EFFECT_TYPE_ROTATE_X, EFFECT_TYPE_ROTATE_Y, EFFECT_TYPE_ROTATE_Z, EFFECT_TYPE_ZOOM };

  CAnimEffect(const TiXmlElement *node, EFFECT_TYPE effect);
  CAnimEffect(unsigned int delay, unsigned int length, EFFECT_TYPE effect);
  CAnimEffect(const CAnimEffect &src);

  virtual ~CAnimEffect();
  CAnimEffect& operator=(const CAnimEffect &src);

  void Calculate(unsigned int time, const CPoint &center);
  void ApplyState(ANIMATION_STATE state, const CPoint &center);

  unsigned int GetDelay() const { return m_delay; };
  unsigned int GetLength() const { return m_delay + m_length; };
  const TransformMatrix &GetTransform() const { return m_matrix; };
  EFFECT_TYPE GetType() const { return m_effect; };

  static std::shared_ptr<Tweener> GetTweener(const TiXmlElement *pAnimationNode);
protected:
  TransformMatrix m_matrix;
  EFFECT_TYPE m_effect;

private:
  virtual void ApplyEffect(float offset, const CPoint &center)=0;

  // timing variables
  unsigned int m_length;
  unsigned int m_delay;

  std::shared_ptr<Tweener> m_pTweener;
};

class CFadeEffect : public CAnimEffect
{
public:
  CFadeEffect(const TiXmlElement *node, bool reverseDefaults);
  CFadeEffect(float start, float end, unsigned int delay, unsigned int length);
  virtual ~CFadeEffect() {};
private:
  virtual void ApplyEffect(float offset, const CPoint &center);

  float m_startAlpha;
  float m_endAlpha;
};

class CSlideEffect : public CAnimEffect
{
public:
  CSlideEffect(const TiXmlElement *node);
  virtual ~CSlideEffect() {};
private:
  virtual void ApplyEffect(float offset, const CPoint &center);

  float m_startX;
  float m_startY;
  float m_endX;
  float m_endY;
};

class CRotateEffect : public CAnimEffect
{
public:
  CRotateEffect(const TiXmlElement *node, EFFECT_TYPE effect);
  virtual ~CRotateEffect() {};
private:
  virtual void ApplyEffect(float offset, const CPoint &center);

  float m_startAngle;
  float m_endAngle;

  bool m_autoCenter;
  CPoint m_center;
};

class CZoomEffect : public CAnimEffect
{
public:
  CZoomEffect(const TiXmlElement *node, const CRect &rect);
  virtual ~CZoomEffect() {};
private:
  virtual void ApplyEffect(float offset, const CPoint &center);

  float m_startX;
  float m_startY;
  float m_endX;
  float m_endY;

  bool m_autoCenter;
  CPoint m_center;
};

class CAnimation
{
public:
  CAnimation();
  CAnimation(const CAnimation &src);

  virtual ~CAnimation();

  CAnimation& operator=(const CAnimation &src);

  static CAnimation CreateFader(float start, float end, unsigned int delay, unsigned int length, ANIMATION_TYPE type = ANIM_TYPE_NONE);

  void Create(const TiXmlElement *node, const CRect &rect, int context);

  void Animate(unsigned int time, bool startAnim);
  void ResetAnimation();
  void ApplyAnimation();
  inline void RenderAnimation(TransformMatrix &matrix)
  {
    RenderAnimation(matrix, CPoint());
  }
  void RenderAnimation(TransformMatrix &matrix, const CPoint &center);
  void QueueAnimation(ANIMATION_PROCESS process);

  inline bool IsReversible() const { return m_reversible; };
  inline ANIMATION_TYPE GetType() const { return m_type; };
  inline ANIMATION_STATE GetState() const { return m_currentState; };
  inline ANIMATION_PROCESS GetProcess() const { return m_currentProcess; };
  inline ANIMATION_PROCESS GetQueuedProcess() const { return m_queuedProcess; };

  bool CheckCondition();
  void UpdateCondition(const CGUIListItem *item = NULL);
  void SetInitialCondition();

private:
  void Calculate(const CPoint &point);
  void AddEffect(const CStdString &type, const TiXmlElement *node, const CRect &rect);

  enum ANIM_REPEAT { ANIM_REPEAT_NONE = 0, ANIM_REPEAT_PULSE, ANIM_REPEAT_LOOP };

  // type of animation
  ANIMATION_TYPE m_type;
  bool m_reversible;
  INFO::InfoPtr m_condition;

  // conditional anims can repeat
  ANIM_REPEAT m_repeatAnim;
  bool m_lastCondition;

  // state of animation
  ANIMATION_PROCESS m_queuedProcess;
  ANIMATION_PROCESS m_currentProcess;
  ANIMATION_STATE m_currentState;

  // timing of animation
  unsigned int m_start;
  unsigned int m_length;
  unsigned int m_delay;
  unsigned int m_amount;

  std::vector<CAnimEffect *> m_effects;
};

/**
 * Class used to handle scrolling, allow using tweeners.
 * Usage:
 *   start scrolling using ScrollTo() method / stop scrolling using Stop() method
 *   update scroll value each frame with current time using Update() method
 *   get/set scroll value using GetValue()/SetValue()
 */
class CScroller
{
public:
  CScroller(unsigned int duration = 200, std::shared_ptr<Tweener> tweener = std::shared_ptr<Tweener>());
  CScroller(const CScroller& right);
  CScroller& operator=(const CScroller &src);
  ~CScroller();

  /**
   * Set target value scroller will be scrolling to
   * @param endPos target 
   */
  void ScrollTo(float endPos);
  
  /**
   * Immediately stop scrolling
   */
  void Stop() { m_delta = 0; };
  /**
   * Update the scroller to where it would be at the given time point, calculating a new Value.
   * @param time time point
   * @return True if we are scrolling at given time point
   */
  bool Update(unsigned int time);

  /**
   * Value of scroll
   */
  float GetValue() const { return m_scrollValue; };
  void SetValue(float scrollValue) { m_scrollValue = scrollValue; };

  bool IsScrolling() const { return m_delta != 0; };
  bool IsScrollingUp() const { return m_delta < 0; };
  bool IsScrollingDown() const { return m_delta > 0; };

  unsigned int GetDuration() const { return m_duration; };
private:
  float Tween(float progress);

  float        m_scrollValue;
  float        m_delta;                   //!< Brief distance that we have to travel during scroll
  float        m_startPosition;           //!< Brief starting position of scroll
  bool         m_hasResumePoint;          //!< Brief check if we should tween from middle of the tween
  unsigned int m_startTime;               //!< Brief starting time of scroll
  unsigned int m_lastTime;                //!< Brief last remember time (updated each time Scroll() method is called)

  unsigned int m_duration;                //!< Brief duration of scroll
  std::shared_ptr<Tweener> m_pTweener;
};
