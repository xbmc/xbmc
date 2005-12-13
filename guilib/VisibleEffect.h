#pragma once

enum EFFECT_TYPE { EFFECT_TYPE_NONE = 0, EFFECT_TYPE_FADE, EFFECT_TYPE_SLIDE };
enum EFFECT_STATE { EFFECT_NONE = 0, EFFECT_IN, EFFECT_OUT };
enum START_STATE { START_NONE = 0, START_HIDDEN, START_VISIBLE };

#include "include.h"

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
