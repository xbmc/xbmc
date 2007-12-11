#pragma once

enum EFFECT_TYPE { EFFECT_TYPE_NONE = 0, EFFECT_TYPE_FADE, EFFECT_TYPE_SLIDE, EFFECT_TYPE_ROTATE_X, EFFECT_TYPE_ROTATE_Y, EFFECT_TYPE_ROTATE_Z, EFFECT_TYPE_ZOOM };
enum ANIMATION_PROCESS { ANIM_PROCESS_NONE = 0, ANIM_PROCESS_NORMAL, ANIM_PROCESS_REVERSE };
enum ANIMATION_STATE { ANIM_STATE_NONE = 0, ANIM_STATE_DELAYED, ANIM_STATE_IN_PROCESS, ANIM_STATE_APPLIED };

// forward definitions

class TiXmlElement;
class Tweener;
struct FRECT;
class CGUIListItem;

#include "TransformMatrix.h"  // needed for the TransformMatrix member

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

class CAnimation
{
public:
  CAnimation();
  CAnimation(const CAnimation&);
  virtual ~CAnimation();
  CAnimation &operator=(const CAnimation &src);

  void Reset();
  void Create(const TiXmlElement *node, const FRECT &rect);
  static CAnimation *CreateFader(float start, float end, unsigned int delay, unsigned int length);
  void Animate(unsigned int time, bool startAnim);
  void ResetAnimation();
  void ApplyAnimation();
  void RenderAnimation(TransformMatrix &matrix);
  void QueueAnimation(ANIMATION_PROCESS process);

  inline bool IsReversible() const { return m_reversible; };
  inline int  GetCondition() const { return m_condition; };
  inline ANIMATION_TYPE GetType() const { return m_type; };
  inline ANIMATION_STATE GetState() const { return m_currentState; };
  inline ANIMATION_PROCESS GetProcess() const { return m_currentProcess; };
  inline ANIMATION_PROCESS GetQueuedProcess() const { return m_queuedProcess; };

  float m_amount;

  void UpdateCondition(DWORD contextWindow, const CGUIListItem *item = NULL);
  void SetInitialCondition(DWORD contextWindow);

private:
  enum ANIM_REPEAT { ANIM_REPEAT_NONE = 0, ANIM_REPEAT_PULSE, ANIM_REPEAT_LOOP };

  ANIMATION_TYPE m_type;
  EFFECT_TYPE m_effect;

  ANIMATION_STATE m_currentState;
  ANIMATION_PROCESS m_currentProcess;
  ANIMATION_PROCESS m_queuedProcess;

  // animation variables
  float m_startX;
  float m_startY;
  float m_endX;
  float m_endY;
  float m_centerX;
  float m_centerY;
  float m_startAlpha;
  float m_endAlpha;

  // timing variables
  unsigned int m_start;
  unsigned int m_length;
  unsigned int m_delay;
  ANIM_REPEAT m_repeatAnim;
  bool m_reversible;    // whether the animation is reversible or not

  int m_condition;      // conditions that must be satisfied in order for this
                      // animation to be performed
  bool m_lastCondition; // last state of our conditional

  void Calculate();
  TransformMatrix m_matrix;
  Tweener *m_pTweener;
};