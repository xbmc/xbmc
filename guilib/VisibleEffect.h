#pragma once

enum EFFECT_TYPE { EFFECT_TYPE_NONE = 0, EFFECT_TYPE_FADE, EFFECT_TYPE_SLIDE, EFFECT_TYPE_ROTATE, EFFECT_TYPE_ZOOM };
enum ANIMATION_PROCESS { ANIM_PROCESS_NONE = 0, ANIM_PROCESS_NORMAL, ANIM_PROCESS_REVERSE };
enum ANIMATION_STATE { ANIM_STATE_NONE = 0, ANIM_STATE_DELAYED, ANIM_STATE_IN_PROCESS, ANIM_STATE_APPLIED };

// forward definitions

class TiXmlElement;
struct FRECT;

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
  void Reset();
  void Create(const TiXmlElement *node, const FRECT &rect);
  void CreateReverse(const CAnimation &anim);
  void Animate(unsigned int time, bool startAnim);
  void ResetAnimation();
  void ApplyAnimation();
  void RenderAnimation(TransformMatrix &matrix);

  ANIMATION_TYPE type;
  EFFECT_TYPE effect;

  ANIMATION_PROCESS queuedProcess;
  ANIMATION_STATE currentState;
  ANIMATION_PROCESS currentProcess;

  int condition;      // conditions that must be satisfied in order for this
                      // animation to be performed
  bool lastCondition; // last state of our conditional

  inline bool IsReversible() const { return reversible; };

  float amount;
private:
  // animation variables
  float acceleration;
  float startX;
  float startY;
  float endX;
  float endY;
  float centerX;
  float centerY;
  int startAlpha;
  int endAlpha;

  // timing variables
  unsigned int start;
  unsigned int length;
  unsigned int delay;

  bool reversible;    // whether the animation is reversible or not

  void Calculate();
  TransformMatrix m_matrix;
};