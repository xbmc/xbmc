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

CAnimation::CAnimation()
{
  Reset();
}

void CAnimation::Reset()
{
  type = ANIM_TYPE_NONE;
  effect = EFFECT_TYPE_NONE;
  currentState = ANIM_STATE_NONE;
  currentProcess = queuedProcess = ANIM_PROCESS_NONE;
  amount = 0;
  delay = start = length = 0;
  startX = startY = endX = endY = 0;
  startAlpha = 0;
  endAlpha = 100;
  acceleration = 0;
}

void CAnimation::Create(TiXmlElement *node, RESOLUTION res)
{
  if (!node || !node->FirstChild())
    return;
  const char *animType = node->FirstChild()->Value();
  if (strcmpi(animType, "visible") == 0)
    type = ANIM_TYPE_VISIBLE;
  else if (strcmpi(animType, "hidden") == 0)
    type = ANIM_TYPE_HIDDEN;
  else if (strcmpi(animType, "visiblechange") == 0)
    type = ANIM_TYPE_VISIBLE;
  else if (strcmpi(animType, "focus") == 0)
    type = ANIM_TYPE_FOCUS;
  else if (strcmpi(animType, "unfocus") == 0)
    type = ANIM_TYPE_UNFOCUS;
  else if (strcmpi(animType, "windowopen") == 0)
    type = ANIM_TYPE_WINDOW_OPEN;
  else if (strcmpi(animType, "windowclose") == 0)
    type = ANIM_TYPE_WINDOW_CLOSE;
  if (type == ANIM_TYPE_NONE)
  {
    CLog::Log(LOGERROR, "Control has invalid animation type");
    return;
  }
  const char *effectType = node->Attribute("effect");
  if (!effectType)
    return;
  // effect type
  if (effectType && strcmpi(effectType, "fade") == 0)
    effect = EFFECT_TYPE_FADE;
  else if (effectType && strcmpi(effectType, "slide") == 0)
    effect = EFFECT_TYPE_SLIDE;
  // time and delay
  node->Attribute("time", (int *)&length);
  node->Attribute("delay", (int *)&delay);
  // slide parameters
  if (effect == EFFECT_TYPE_SLIDE)
  {
    const char *startPos = node->Attribute("start");
    if (startPos)
    {
      startX = atoi(startPos);
      const char *comma = strstr(startPos, ",");
      if (comma)
        startY = atoi(comma + 1);
    }
    const char *endPos = node->Attribute("end");
    if (endPos)
    {
      endX = atoi(endPos);
      const char *comma = strstr(endPos, ",");
      if (comma)
        endY = atoi(comma + 1);
    }
    double accel;
    if (node->Attribute("accleration", &accel)) acceleration = (float)accel;
    if (acceleration > 1.0f) acceleration = 1.0f;
    if (acceleration < -1.0f) acceleration = -1.0f;
    // scale our parameters
    g_graphicsContext.ScaleXCoord(startX, res);
    g_graphicsContext.ScaleYCoord(startY, res);
    g_graphicsContext.ScaleXCoord(endX, res);
    g_graphicsContext.ScaleYCoord(endY, res);
  }
  else
  {  // alpha parameters
    if (type < 0)
    { // out effect defaults
      startAlpha = 100;
      endAlpha = 0;
    }
    else
    { // in effect defaults
      startAlpha = 0;
      endAlpha = 100;
    }
    if (node->Attribute("start")) node->Attribute("start", &startAlpha);
    if (node->Attribute("end")) node->Attribute("end", &endAlpha);
    if (startAlpha > 100) startAlpha = 100;
    if (endAlpha > 100) endAlpha = 100;
    if (startAlpha < 0) startAlpha = 0;
    if (endAlpha < 0) endAlpha = 0;
  }
}

// creates the reverse animation
void CAnimation::CreateReverse(const CAnimation &anim)
{
  acceleration = -anim.acceleration;
  startX = anim.endX;
  startY = anim.endY;
  endX = anim.startX;
  endY = anim.startY;
  endAlpha = anim.startAlpha;
  startAlpha = anim.endAlpha;
  type = (ANIMATION_TYPE)-anim.type;
  effect = anim.effect;
  length = anim.length;
}

void CAnimation::Animate(DWORD time, bool hasRendered)
{
  // First start any queued animations
  if (queuedProcess == ANIM_PROCESS_NORMAL)
  {
    if (currentProcess == ANIM_PROCESS_REVERSE)
      start = time - (int)(length * amount);  // reverse direction of effect
    else
      start = time;
    currentProcess = ANIM_PROCESS_NORMAL;
  }
  else if (queuedProcess == ANIM_PROCESS_REVERSE)
  {
    if (currentProcess == ANIM_PROCESS_NORMAL)
      start = time - (int)(length * (1 - amount)); // turn around direction of effect
    else
      start = time;
    currentProcess = ANIM_PROCESS_REVERSE;
  }
  // reset the queued state once we've rendered
  // Note that if we are delayed, then the resource may not have been allocated as yet
  // as it hasn't been rendered (is still invisible).  Ideally, the resource should
  // be allocated based on a visible state, rather than a bool on/off, then only rendered
  // if it's in the appropriate state (ie allow visible = NO, DELAYED, VISIBLE, and allocate
  // if it's not NO, render if it's VISIBLE)  The alternative, is to just always render
  // the control while it's in the DELAYED state (comes down to the definition of the states)
  if (hasRendered || queuedProcess == ANIM_PROCESS_REVERSE
      || (currentState == ANIM_STATE_DELAYED && type > 0))
    queuedProcess = ANIM_PROCESS_NONE;
  // Update our animation process
  if (currentProcess != ANIM_PROCESS_NONE)
  {
    if (time - start < delay)
    {
      amount = 0.0f;
      currentState = ANIM_STATE_DELAYED;
    }
    else if (time - start < length + delay)
    {
      amount = (float)(time - start - delay) / length;
      currentState = ANIM_STATE_IN_PROCESS;
    }
    else
    {
      amount = 1.0f;
      currentState = ANIM_STATE_APPLIED;
    }
  }
  if (currentProcess == ANIM_PROCESS_REVERSE)
    amount = 1.0f - amount;
}

CAttribute CAnimation::RenderAnimation()
{
  // If we have finished an animation, reset the animation state
  // We do this here (rather than in Animate()) as we need the
  // currentProcess information in the UpdateStates() function of the
  // window and control classes.
  if (currentState == ANIM_STATE_APPLIED)
    currentProcess = ANIM_PROCESS_NONE;
  // Now do the real animation
  CAttribute attribute;
  if (currentProcess != ANIM_PROCESS_NONE || currentState == ANIM_STATE_APPLIED)
  {
    if (effect == EFFECT_TYPE_FADE)
      attribute.alpha = (DWORD)(((float)(endAlpha - startAlpha) * amount + startAlpha) * 2.55f);
    else if (effect == EFFECT_TYPE_SLIDE)
    {
      float offset = amount * (acceleration * amount + 1.0f - acceleration);
      attribute.offsetX = (int)((endX - startX)*offset + startX);
      attribute.offsetY = (int)((endY - startY)*offset + startY);
    }
  }
  return attribute;
}

void CAnimation::ResetAnimation()
{
  currentProcess = ANIM_PROCESS_NONE;
  queuedProcess = ANIM_PROCESS_NONE;
  currentState = ANIM_STATE_NONE;
}