#include "include.h"
#include "VisibleEffect.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "SkinInfo.h" // for the effect time adjustments
#include "GUIImage.h" // for FRECT

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
  centerX = centerY = 0;
  startAlpha = 0;
  endAlpha = 100;
  acceleration = 0;
  condition = 0;
  reversible = true;
  lastCondition = false;
}

void CAnimation::Create(const TiXmlElement *node, const FRECT &rect)
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
  else if (strcmpi(animType, "conditional") == 0)
    type = ANIM_TYPE_CONDITIONAL;
  if (type == ANIM_TYPE_NONE)
  {
    CLog::Log(LOGERROR, "Control has invalid animation type");
    return;
  }
  const char *conditionString = node->Attribute("condition");
  if (conditionString)
    condition = g_infoManager.TranslateString(conditionString);
  const char *effectType = node->Attribute("effect");
  if (!effectType)
    return;
  // effect type
  if (strcmpi(effectType, "fade") == 0)
    effect = EFFECT_TYPE_FADE;
  else if (strcmpi(effectType, "slide") == 0)
    effect = EFFECT_TYPE_SLIDE;
  else if (strcmpi(effectType, "rotate") == 0)
    effect = EFFECT_TYPE_ROTATE;
  else if (strcmpi(effectType, "zoom") == 0)
    effect = EFFECT_TYPE_ZOOM;
  // time and delay
  node->Attribute("time", (int *)&length);
  node->Attribute("delay", (int *)&delay);
  length = (unsigned int)(length * g_SkinInfo.GetEffectsSlowdown());
  delay = (unsigned int)(delay * g_SkinInfo.GetEffectsSlowdown());
  // reversible (defaults to true)
  const char *reverse = node->Attribute("reversible");
  if (reverse && strcmpi(reverse, "false") == 0)
    reversible = false;
  // acceleration of effect
  double accel;
  if (node->Attribute("acceleration", &accel)) acceleration = (float)accel;
  // slide parameters
  if (effect == EFFECT_TYPE_SLIDE)
  {
    const char *startPos = node->Attribute("start");
    if (startPos)
    {
      startX = (float)atof(startPos);
      const char *comma = strstr(startPos, ",");
      if (comma)
        startY = (float)atof(comma + 1);
    }
    const char *endPos = node->Attribute("end");
    if (endPos)
    {
      endX = (float)atof(endPos);
      const char *comma = strstr(endPos, ",");
      if (comma)
        endY = (float)atof(comma + 1);
    }
  }
  else if (effect == EFFECT_TYPE_FADE)
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
  else if (effect == EFFECT_TYPE_ROTATE)
  {
    double temp;
    if (node->Attribute("start", &temp)) startX = (float)temp;
    if (node->Attribute("end", &temp)) endX = (float)temp;

    // convert to a negative to account for our reversed vertical axis
    startX *= -1;
    endX *= -1;

    const char *centerPos = node->Attribute("center");
    if (centerPos)
    {
      centerX = (float)atof(centerPos);
      const char *comma = strstr(centerPos, ",");
      if (comma)
        centerY = (float)atof(comma + 1);
    }
  }
  else // if (effect == EFFECT_TYPE_ZOOM)
  {
    // effect defaults
    startX = startY = 100;
    endX = endY = 100;
    centerX = centerY = 0;

    float startPosX = rect.left;
    float startPosY = rect.top;
    float endPosX = rect.left;
    float endPosY = rect.right;

    const char *start = node->Attribute("start");
    if (start)
    {
      CStdStringArray params;
      StringUtils::SplitString(start, ",", params);
      if (params.size() == 1)
        startX = startY = (float)atof(start);
      else if (params.size() == 2)
      {
        startX = (float)atof(params[0].c_str());
        startY = (float)atof(params[1].c_str());
      }
      else if (params.size() == 4)
      { // format is start="x,y,width,height"
        // use width and height from our rect to calculate our sizing
        startPosX = (float)atof(params[0].c_str());
        startPosY = (float)atof(params[1].c_str());
        startX = (float)atof(params[2].c_str()) / rect.right * 100.0f;
        startY = (float)atof(params[3].c_str()) / rect.bottom * 100.0f;
      }
    }
    const char *end = node->Attribute("end");
    if (end)
    {
      CStdStringArray params;
      StringUtils::SplitString(end, ",", params);
      if (params.size() == 1)
        endX = endY = (float)atof(end);
      else if (params.size() == 2)
      {
        endX = (float)atof(params[0].c_str());
        endY = (float)atof(params[1].c_str());
      }
      else if (params.size() == 4)
      { // format is start="x,y,width,height"
        // use width and height from our rect to calculate our sizing
        endPosX = (float)atof(params[0].c_str());
        endPosY = (float)atof(params[1].c_str());
        endX = (float)atof(params[2].c_str()) / rect.right * 100.0f;
        endY = (float)atof(params[3].c_str()) / rect.bottom * 100.0f;
      }
    }
    const char *centerPos = node->Attribute("center");
    if (centerPos)
    {
      centerX = (float)atof(centerPos);
      const char *comma = strstr(centerPos, ",");
      if (comma)
        centerY = (float)atof(comma + 1);
    }
    else
    { // no center specified
      // calculate the center position...
      if (startX)
      {
        float scale = endX / startX;
        if (scale != 1)
          centerX = (endPosX - scale*startPosX) / (1 - scale);
      }
      if (startY)
      {
        float scale = endY / startY;
        if (scale != 1)
          centerY = (endPosY - scale*startPosY) / (1 - scale);
      }
    }
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
  centerX = anim.centerX;
  centerY = anim.centerY;
  type = (ANIMATION_TYPE)-anim.type;
  effect = anim.effect;
  length = anim.length;
  reversible = anim.reversible;
}

void CAnimation::Animate(unsigned int time, bool startAnim)
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
  // reset the queued state once we've rendered to ensure allocation has occured
  if (startAnim || queuedProcess == ANIM_PROCESS_REVERSE)// || (currentState == ANIM_STATE_DELAYED && type > 0))
    queuedProcess = ANIM_PROCESS_NONE;

  // Update our animation process
  if (currentProcess == ANIM_PROCESS_NORMAL)
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
  else if (currentProcess == ANIM_PROCESS_REVERSE)
  {
    if (time - start < length)
    {
      amount = 1.0f - (float)(time - start) / length;
      currentState = ANIM_STATE_IN_PROCESS;
    }
    else
    {
      amount = 0.0f;
      currentState = ANIM_STATE_APPLIED;
    }
  }
}

#define DEGREE_TO_RADIAN 0.01745329252f

void CAnimation::RenderAnimation(TransformMatrix &matrix)
{
  // If we have finished an animation, reset the animation state
  // We do this here (rather than in Animate()) as we need the
  // currentProcess information in the UpdateStates() function of the
  // window and control classes.

  // Now do the real animation
  if (currentProcess != ANIM_PROCESS_NONE)
    Calculate();
  if (currentState == ANIM_STATE_APPLIED)
  {
    currentProcess = ANIM_PROCESS_NONE;
    queuedProcess = ANIM_PROCESS_NONE;
  }
  if (currentState != ANIM_STATE_NONE)
    matrix *= m_matrix;
}

void CAnimation::Calculate()
{
  float offset = amount * (acceleration * amount + 1.0f - acceleration);
  if (effect == EFFECT_TYPE_FADE)
    m_matrix.SetFader(((float)(endAlpha - startAlpha) * amount + startAlpha) * 0.01f);
  else if (effect == EFFECT_TYPE_SLIDE)
  {
    m_matrix.SetTranslation((endX - startX)*offset + startX, (endY - startY)*offset + startY);
  }
  else if (effect == EFFECT_TYPE_ROTATE)
  {
    m_matrix.SetTranslation(centerX, centerY);
    m_matrix *= TransformMatrix::CreateRotation(((endX - startX)*offset + startX) * DEGREE_TO_RADIAN);
    m_matrix *= TransformMatrix::CreateTranslation(-centerX, -centerY);
  }
  else if (effect == EFFECT_TYPE_ZOOM)
  {
    float scaleX = ((endX - startX)*offset + startX) * 0.01f;
    float scaleY = ((endY - startY)*offset + startY) * 0.01f;
    m_matrix.SetTranslation(centerX, centerY);
    m_matrix *= TransformMatrix::CreateScaler(scaleX, scaleY);
    m_matrix *= TransformMatrix::CreateTranslation(-centerX, -centerY);
  }
}
void CAnimation::ResetAnimation()
{
  currentProcess = ANIM_PROCESS_NONE;
  queuedProcess = ANIM_PROCESS_NONE;
  currentState = ANIM_STATE_NONE;
}

void CAnimation::ApplyAnimation()
{
  currentProcess = ANIM_PROCESS_NONE;
  queuedProcess = ANIM_PROCESS_NONE;
  currentState = ANIM_STATE_APPLIED;
  amount = 1.0f;
  Calculate();
}