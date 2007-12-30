#pragma once

enum ANIMATION_PROCESS { ANIM_PROCESS_NONE = 0, ANIM_PROCESS_NORMAL, ANIM_PROCESS_REVERSE };
enum ANIMATION_STATE { ANIM_STATE_NONE = 0, ANIM_STATE_DELAYED, ANIM_STATE_IN_PROCESS, ANIM_STATE_APPLIED };

// forward definitions

class TiXmlElement;
class Tweener;
struct FRECT;
class CGUIListItem;

#include "TransformMatrix.h"  // needed for the TransformMatrix member
#include "Geometry.h"         // for CPoint

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
  virtual ~CAnimEffect();
  void Free();
  void AddReference();

  void Calculate(unsigned int time, const CPoint &center);
  void ApplyState(ANIMATION_STATE state, const CPoint &center);

  unsigned int GetDelay() const { return m_delay; };
  unsigned int GetLength() const { return m_delay + m_length; };
  const TransformMatrix &GetTransform() const { return m_matrix; };

protected:
  TransformMatrix m_matrix;
  EFFECT_TYPE m_effect;

private:
  virtual void ApplyEffect(float offset, const CPoint &center)=0;

  // timing variables
  unsigned int m_length;
  unsigned int m_delay;

  Tweener *m_pTweener;
  // reference tracking
  unsigned int m_referenceCount;
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
  CZoomEffect(const TiXmlElement *node, const FRECT &rect);
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

  const CAnimation &CAnimation::operator=(const CAnimation &src);

  static CAnimation *CreateFader(float start, float end, unsigned int delay, unsigned int length);

  void Create(const TiXmlElement *node, const FRECT &rect);

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
  inline int  GetCondition() const { return m_condition; };
  inline ANIMATION_TYPE GetType() const { return m_type; };
  inline ANIMATION_STATE GetState() const { return m_currentState; };
  inline ANIMATION_PROCESS GetProcess() const { return m_currentProcess; };
  inline ANIMATION_PROCESS GetQueuedProcess() const { return m_queuedProcess; };

  void UpdateCondition(DWORD contextWindow, const CGUIListItem *item = NULL);
  void SetInitialCondition(DWORD contextWindow);

private:
  void Calculate(const CPoint &point);
  void AddEffect(const CStdString &type, const TiXmlElement *node, const FRECT &rect);
  void AddEffect(CAnimEffect *effect);

  enum ANIM_REPEAT { ANIM_REPEAT_NONE = 0, ANIM_REPEAT_PULSE, ANIM_REPEAT_LOOP };

  // type of animation
  ANIMATION_TYPE m_type;
  bool m_reversible;
  int m_condition;

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

  vector<CAnimEffect *> m_effects;
};