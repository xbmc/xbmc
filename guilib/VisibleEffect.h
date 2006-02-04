#pragma once

enum EFFECT_TYPE { EFFECT_TYPE_NONE = 0, EFFECT_TYPE_FADE, EFFECT_TYPE_SLIDE, EFFECT_TYPE_ROTATE, EFFECT_TYPE_ZOOM };
enum ANIMATION_PROCESS { ANIM_PROCESS_NONE = 0, ANIM_PROCESS_NORMAL, ANIM_PROCESS_REVERSE };
enum ANIMATION_STATE { ANIM_STATE_NONE = 0, ANIM_STATE_DELAYED, ANIM_STATE_IN_PROCESS, ANIM_STATE_APPLIED };

#include "include.h"
#include "GraphicContext.h"

enum START_STATE { START_NONE = 0, START_HIDDEN, START_VISIBLE };
enum EFFECT_STATE { EFFECT_NONE = 0, EFFECT_DELAYED, EFFECT_IN, EFFECT_OUT, EFFECT_APPLIED };

class CVisibleEffect
{
public:
  CVisibleEffect();
  void Create(TiXmlElement *node);

  EFFECT_TYPE m_type;
  START_STATE m_startState;
  DWORD m_inTime;           // in ms.
  DWORD m_outTime;
  DWORD m_inDelay;
  DWORD m_outDelay;
  EFFECT_STATE m_state;
  bool m_allowHiddenFocus;  // true if we should be focusable even if hidden

  int m_startX;           // for slide effects
  int m_startY;
  float m_acceleration;
};

enum ANIMATION_TYPE
{
  ANIM_TYPE_UNFOCUS = -3,
  ANIM_TYPE_HIDDEN,
  ANIM_TYPE_WINDOW_CLOSE,
  ANIM_TYPE_NONE,
  ANIM_TYPE_WINDOW_OPEN,
  ANIM_TYPE_VISIBLE,
  ANIM_TYPE_FOCUS,
};

class CAnimation
{
public:
  CAnimation();
  void Reset();
  void Create(TiXmlElement *node, RESOLUTION res);
  void CreateReverse(const CAnimation &anim);
  void Animate(DWORD time, bool hasRendered);
  void ResetAnimation();
  TransformMatrix RenderAnimation();

  ANIMATION_TYPE type;
  EFFECT_TYPE effect;
  int startX;
  int startY;
  int endX;
  int endY;
  int startAlpha;
  int endAlpha;
  float acceleration;

  unsigned int length;
  unsigned int delay;
// for debug
  float amount;
// for debug

  ANIMATION_PROCESS queuedProcess;
  ANIMATION_STATE currentState;
  ANIMATION_PROCESS currentProcess;

private:
//  float amount;
  // timing variables
  unsigned int start;
};