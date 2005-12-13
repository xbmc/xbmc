#pragma once

enum EFFECT_TYPE { EFFECT_TYPE_NONE = 0, EFFECT_TYPE_FADE, EFFECT_TYPE_SLIDE };
enum EFFECT_STATE { EFFECT_NONE = 0, EFFECT_IN, EFFECT_OUT };
enum START_STATE { START_NONE = 0, START_HIDDEN, START_VISIBLE };

class CVisibleEffect
{
public:
  CVisibleEffect()
  {
    m_type = EFFECT_TYPE_NONE;
    m_startState = START_NONE;
    m_inDelay = m_inTime = m_outDelay = m_outTime = 0;
    m_startX = m_startY = 0;
    m_acceleration = 1;
    m_allowHiddenFocus = false;
  }
  void Create(TiXmlElement *node)
  {
    const char *effectType = node->Attribute("effect");
    if (!effectType)
      return;

    // effect type
    if (effectType && strcmpi(effectType, "fade") == 0)
      m_type = EFFECT_TYPE_FADE;
    else if (effectType && strcmpi(effectType, "slide") == 0)
      m_type = EFFECT_TYPE_SLIDE;
    // start state
    const char *start = node->Attribute("start");
    if (start && !strcmpi(start, "hidden"))
      m_startState = START_HIDDEN;
    if (start && !strcmpi(start, "visible"))
      m_startState = START_VISIBLE;
    // time
    node->Attribute("time", (int *)&m_inTime);
    m_outTime = m_inTime;
    int tempTime;
    if (node->Attribute("intime", &tempTime)) m_inTime = tempTime;
    if (node->Attribute("outtime", &tempTime)) m_outTime = tempTime;
    // delay
    node->Attribute("delay", (int *)&m_inDelay);
    m_outDelay = m_inDelay;
    if (node->Attribute("indelay", &tempTime)) m_inDelay = tempTime;
    if (node->Attribute("outdelay", &tempTime)) m_outDelay = tempTime;
    // slide parameters
    node->Attribute("startX", &m_startX);
    node->Attribute("startY", &m_startY);
    double accel;
    if (node->Attribute("accleration", &accel)) m_acceleration = (float)accel;
    // focus when hidden
    const char *focus = node->Attribute("allowhiddenfocus");
    if (focus && strcmp(focus, "true"))
      m_allowHiddenFocus = true;
  };

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
