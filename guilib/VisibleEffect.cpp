#include "VisibleEffect.h"

CVisibleEffect::CVisibleEffect()
{
  m_type = EFFECT_TYPE_NONE;
  m_startState = START_NONE;
  m_inDelay = m_inTime = m_outDelay = m_outTime = 0;
  m_startX = m_startY = 0;
  m_acceleration = 1;
  m_allowHiddenFocus = false;
}

void CVisibleEffect::Create(TiXmlElement *node)
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
  node->Attribute("startx", &m_startX);
  node->Attribute("starty", &m_startY);
  double accel;
  if (node->Attribute("accleration", &accel)) m_acceleration = (float)accel;
  // focus when hidden
  const char *focus = node->Attribute("allowhiddenfocus");
  if (focus && strcmpi(focus, "true") == 0)
    m_allowHiddenFocus = true;
}

