#pragma once

enum EFFECT_TYPE { EFFECT_TYPE_NONE = 0, EFFECT_TYPE_FADE, EFFECT_TYPE_SLIDE, EFFECT_TYPE_ROTATE, EFFECT_TYPE_ZOOM };
enum ANIMATION_PROCESS { ANIM_PROCESS_NONE = 0, ANIM_PROCESS_NORMAL, ANIM_PROCESS_REVERSE };
enum ANIMATION_STATE { ANIM_STATE_NONE = 0, ANIM_STATE_DELAYED, ANIM_STATE_IN_PROCESS, ANIM_STATE_APPLIED };

#include "include.h"
#include "GraphicContext.h"

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
  void Create(const TiXmlElement *node, RESOLUTION res);
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

  int condition;      // conditions that must be satisfied in order for this
                      // animation to be performed

  inline bool IsReversible() const { return reversible; };

private:
//  float amount;
  // timing variables
  unsigned int start;
  bool reversible;    // whether the animation is reversible or not
};