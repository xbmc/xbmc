#include "include.h"
#include "VisibleEffect.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "SkinInfo.h" // for the effect time adjustments
#include "guiImage.h" // for FRECT

CAnimation::CAnimation()
{
  Reset();
}

void CAnimation::Reset()
{
  m_type = ANIM_TYPE_NONE;
  m_effect = EFFECT_TYPE_NONE;
  m_currentState = ANIM_STATE_NONE;
  m_currentProcess = m_queuedProcess = ANIM_PROCESS_NONE;
  m_amount = 0;
  m_delay = m_start = m_length = 0;
  m_startX = m_startY = m_endX = m_endY = 0;
  m_centerX = m_centerY = 0;
  m_startAlpha = 0;
  m_endAlpha = 100;
  m_acceleration = 0;
  m_condition = 0;
  m_reversible = true;
  m_lastCondition = false;
  m_repeatAnim = ANIM_REPEAT_NONE;
}

void CAnimation::Create(const TiXmlElement *node, const FRECT &rect)
{
  if (!node || !node->FirstChild())
    return;
  const char *type = node->FirstChild()->Value();
  if (strcmpi(type, "visible") == 0)
    m_type = ANIM_TYPE_VISIBLE;
  else if (strcmpi(type, "hidden") == 0)
    m_type = ANIM_TYPE_HIDDEN;
  else if (strcmpi(type, "visiblechange") == 0)
    m_type = ANIM_TYPE_VISIBLE;
  else if (strcmpi(type, "focus") == 0)
    m_type = ANIM_TYPE_FOCUS;
  else if (strcmpi(type, "unfocus") == 0)
    m_type = ANIM_TYPE_UNFOCUS;
  else if (strcmpi(type, "windowopen") == 0)
    m_type = ANIM_TYPE_WINDOW_OPEN;
  else if (strcmpi(type, "windowclose") == 0)
    m_type = ANIM_TYPE_WINDOW_CLOSE;
  else if (strcmpi(type, "conditional") == 0)
    m_type = ANIM_TYPE_CONDITIONAL;
  if (m_type == ANIM_TYPE_NONE)
  {
    CLog::Log(LOGERROR, "Control has invalid animation type");
    return;
  }
  const char *condition = node->Attribute("condition");
  if (condition)
    m_condition = g_infoManager.TranslateString(condition);
  const char *effect = node->Attribute("effect");
  if (!effect)
    return;
  // effect type
  if (strcmpi(effect, "fade") == 0)
    m_effect = EFFECT_TYPE_FADE;
  else if (strcmpi(effect, "slide") == 0)
    m_effect = EFFECT_TYPE_SLIDE;
  else if (strcmpi(effect, "rotate") == 0)
    m_effect = EFFECT_TYPE_ROTATE;
  else if (strcmpi(effect, "zoom") == 0)
    m_effect = EFFECT_TYPE_ZOOM;
  // time and delay
  node->Attribute("time", (int *)&m_length);
  node->Attribute("delay", (int *)&m_delay);
  m_length = (unsigned int)(m_length * g_SkinInfo.GetEffectsSlowdown());
  m_delay = (unsigned int)(m_delay * g_SkinInfo.GetEffectsSlowdown());
  // reversible (defaults to true)
  const char *reverse = node->Attribute("reversible");
  if (reverse && strcmpi(reverse, "false") == 0)
    m_reversible = false;
  // acceleration of effect
  double accel;
  if (node->Attribute("acceleration", &accel)) m_acceleration = (float)accel;
  // pulsed animation?
  if (m_type == ANIM_TYPE_CONDITIONAL)
  {
    const char *pulse = node->Attribute("pulse");
    if (pulse && strcmpi(pulse, "true") == 0)
      m_repeatAnim = ANIM_REPEAT_PULSE;
    const char *loop = node->Attribute("loop");
    if (loop && strcmpi(loop, "true") == 0)
      m_repeatAnim = ANIM_REPEAT_LOOP;
  }
  // slide parameters
  if (m_effect == EFFECT_TYPE_SLIDE)
  {
    const char *startPos = node->Attribute("start");
    if (startPos)
    {
      m_startX = (float)atof(startPos);
      const char *comma = strstr(startPos, ",");
      if (comma)
        m_startY = (float)atof(comma + 1);
    }
    const char *endPos = node->Attribute("end");
    if (endPos)
    {
      m_endX = (float)atof(endPos);
      const char *comma = strstr(endPos, ",");
      if (comma)
        m_endY = (float)atof(comma + 1);
    }
  }
  else if (m_effect == EFFECT_TYPE_FADE)
  {  // alpha parameters
    if (m_type < 0)
    { // out effect defaults
      m_startAlpha = 100;
      m_endAlpha = 0;
    }
    else
    { // in effect defaults
      m_startAlpha = 0;
      m_endAlpha = 100;
    }
    if (node->Attribute("start")) node->Attribute("start", &m_startAlpha);
    if (node->Attribute("end")) node->Attribute("end", &m_endAlpha);
    if (m_startAlpha > 100) m_startAlpha = 100;
    if (m_endAlpha > 100) m_endAlpha = 100;
    if (m_startAlpha < 0) m_startAlpha = 0;
    if (m_endAlpha < 0) m_endAlpha = 0;
  }
  else if (m_effect == EFFECT_TYPE_ROTATE)
  {
    double temp;
    if (node->Attribute("start", &temp)) m_startX = (float)temp;
    if (node->Attribute("end", &temp)) m_endX = (float)temp;

    // convert to a negative to account for our reversed vertical axis
    m_startX *= -1;
    m_endX *= -1;

    const char *centerPos = node->Attribute("center");
    if (centerPos)
    {
      m_centerX = (float)atof(centerPos);
      const char *comma = strstr(centerPos, ",");
      if (comma)
        m_centerY = (float)atof(comma + 1);
    }
  }
  else // if (m_effect == EFFECT_TYPE_ZOOM)
  {
    // effect defaults
    m_startX = m_startY = 100;
    m_endX = m_endY = 100;
    m_centerX = m_centerY = 0;

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
        m_startX = m_startY = (float)atof(start);
      else if (params.size() == 2)
      {
        m_startX = (float)atof(params[0].c_str());
        m_startY = (float)atof(params[1].c_str());
      }
      else if (params.size() == 4)
      { // format is start="x,y,width,height"
        // use width and height from our rect to calculate our sizing
        startPosX = (float)atof(params[0].c_str());
        startPosY = (float)atof(params[1].c_str());
        m_startX = (float)atof(params[2].c_str()) / rect.right * 100.0f;
        m_startY = (float)atof(params[3].c_str()) / rect.bottom * 100.0f;
      }
    }
    const char *end = node->Attribute("end");
    if (end)
    {
      CStdStringArray params;
      StringUtils::SplitString(end, ",", params);
      if (params.size() == 1)
        m_endX = m_endY = (float)atof(end);
      else if (params.size() == 2)
      {
        m_endX = (float)atof(params[0].c_str());
        m_endY = (float)atof(params[1].c_str());
      }
      else if (params.size() == 4)
      { // format is start="x,y,width,height"
        // use width and height from our rect to calculate our sizing
        endPosX = (float)atof(params[0].c_str());
        endPosY = (float)atof(params[1].c_str());
        m_endX = (float)atof(params[2].c_str()) / rect.right * 100.0f;
        m_endY = (float)atof(params[3].c_str()) / rect.bottom * 100.0f;
      }
    }
    const char *centerPos = node->Attribute("center");
    if (centerPos)
    {
      m_centerX = (float)atof(centerPos);
      const char *comma = strstr(centerPos, ",");
      if (comma)
        m_centerY = (float)atof(comma + 1);
    }
    else
    { // no center specified
      // calculate the center position...
      if (m_startX)
      {
        float scale = m_endX / m_startX;
        if (scale != 1)
          m_centerX = (endPosX - scale*startPosX) / (1 - scale);
      }
      if (m_startY)
      {
        float scale = m_endY / m_startY;
        if (scale != 1)
          m_centerY = (endPosY - scale*startPosY) / (1 - scale);
      }
    }
  }
}

// creates the reverse animation
void CAnimation::CreateReverse(const CAnimation &anim)
{
  m_acceleration = -anim.m_acceleration;
  m_startX = anim.m_endX;
  m_startY = anim.m_endY;
  m_endX = anim.m_startX;
  m_endY = anim.m_startY;
  m_endAlpha = anim.m_startAlpha;
  m_startAlpha = anim.m_endAlpha;
  m_centerX = anim.m_centerX;
  m_centerY = anim.m_centerY;
  m_type = (ANIMATION_TYPE)-anim.m_type;
  m_effect = anim.m_effect;
  m_length = anim.m_length;
  m_reversible = anim.m_reversible;
}

void CAnimation::Animate(unsigned int time, bool startAnim)
{
  // First start any queued animations
  if (m_queuedProcess == ANIM_PROCESS_NORMAL)
  {
    if (m_currentProcess == ANIM_PROCESS_REVERSE)
      m_start = time - (int)(m_length * m_amount);  // reverse direction of animation
    else
      m_start = time;
    m_currentProcess = ANIM_PROCESS_NORMAL;
  }
  else if (m_queuedProcess == ANIM_PROCESS_REVERSE)
  {
    if (m_currentProcess == ANIM_PROCESS_NORMAL)
      m_start = time - (int)(m_length * (1 - m_amount)); // turn around direction of animation
    else if (m_currentProcess == ANIM_PROCESS_NONE)
      m_start = time;
    m_currentProcess = ANIM_PROCESS_REVERSE;
  }
  // reset the queued state once we've rendered to ensure allocation has occured
  if (startAnim || m_queuedProcess == ANIM_PROCESS_REVERSE)// || (m_currentState == ANIM_STATE_DELAYED && m_type > 0))
    m_queuedProcess = ANIM_PROCESS_NONE;

  // Update our animation process
  if (m_currentProcess == ANIM_PROCESS_NORMAL)
  {
    if (time - m_start < m_delay)
    {
      m_amount = 0.0f;
      m_currentState = ANIM_STATE_DELAYED;
    }
    else if (time - m_start < m_length + m_delay)
    {
      m_amount = (float)(time - m_start - m_delay) / m_length;
      m_currentState = ANIM_STATE_IN_PROCESS;
    }
    else
    {
      m_amount = 1.0f;
      if (m_repeatAnim == ANIM_REPEAT_PULSE && m_lastCondition)
      { // pulsed anims auto-reverse
        m_currentProcess = ANIM_PROCESS_REVERSE;
        m_start = time;
      }
      else if (m_repeatAnim == ANIM_REPEAT_LOOP && m_lastCondition)
      { // looped anims start over
        m_amount = 0.0f;
        m_start = time;
      }
      else
        m_currentState = ANIM_STATE_APPLIED;
    }
  }
  else if (m_currentProcess == ANIM_PROCESS_REVERSE)
  {
    if (time - m_start < m_length)
    {
      m_amount = 1.0f - (float)(time - m_start) / m_length;
      m_currentState = ANIM_STATE_IN_PROCESS;
    }
    else
    {
      m_amount = 0.0f;
      if (m_repeatAnim == ANIM_REPEAT_PULSE && m_lastCondition)
      { // pulsed anims auto-reverse
        m_currentProcess = ANIM_PROCESS_NORMAL;
        m_start = time;
      }
      else
        m_currentState = ANIM_STATE_APPLIED;
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
  if (m_currentProcess != ANIM_PROCESS_NONE)
    Calculate();
  if (m_currentState == ANIM_STATE_APPLIED)
  {
    m_currentProcess = ANIM_PROCESS_NONE;
    m_queuedProcess = ANIM_PROCESS_NONE;
  }
  if (m_currentState != ANIM_STATE_NONE)
    matrix *= m_matrix;
}

void CAnimation::Calculate()
{
  float offset = m_amount * (m_acceleration * m_amount + 1.0f - m_acceleration);
  if (m_effect == EFFECT_TYPE_FADE)
    m_matrix.SetFader(((float)(m_endAlpha - m_startAlpha) * m_amount + m_startAlpha) * 0.01f);
  else if (m_effect == EFFECT_TYPE_SLIDE)
  {
    m_matrix.SetTranslation((m_endX - m_startX)*offset + m_startX, (m_endY - m_startY)*offset + m_startY);
  }
  else if (m_effect == EFFECT_TYPE_ROTATE)
  {
    m_matrix.SetTranslation(m_centerX, m_centerY);
    m_matrix *= TransformMatrix::CreateRotation(((m_endX - m_startX)*offset + m_startX) * DEGREE_TO_RADIAN);
    m_matrix *= TransformMatrix::CreateTranslation(-m_centerX, -m_centerY);
  }
  else if (m_effect == EFFECT_TYPE_ZOOM)
  {
    float scaleX = ((m_endX - m_startX)*offset + m_startX) * 0.01f;
    float scaleY = ((m_endY - m_startY)*offset + m_startY) * 0.01f;
    m_matrix.SetTranslation(m_centerX, m_centerY);
    m_matrix *= TransformMatrix::CreateScaler(scaleX, scaleY);
    m_matrix *= TransformMatrix::CreateTranslation(-m_centerX, -m_centerY);
  }
}
void CAnimation::ResetAnimation()
{
  m_currentProcess = ANIM_PROCESS_NONE;
  m_queuedProcess = ANIM_PROCESS_NONE;
  m_currentState = ANIM_STATE_NONE;
}

void CAnimation::ApplyAnimation()
{
  m_currentProcess = ANIM_PROCESS_NONE;
  m_queuedProcess = ANIM_PROCESS_NONE;
  m_currentState = ANIM_STATE_APPLIED;
  m_amount = 1.0f;
  Calculate();
}

void CAnimation::UpdateCondition()
{
  bool condition = g_infoManager.GetBool(m_condition);
  if (condition && !m_lastCondition)
    m_queuedProcess = ANIM_PROCESS_NORMAL;
  else if (!condition && m_lastCondition)
  {
    if (m_reversible)
      m_queuedProcess = ANIM_PROCESS_REVERSE;
    else
      ResetAnimation();
  }
  m_lastCondition = condition;
}

void CAnimation::SetInitialCondition()
{
  m_lastCondition = g_infoManager.GetBool(m_condition);
  if (m_lastCondition)
    ApplyAnimation();
  else
    ResetAnimation();
}

void CAnimation::QueueAnimation(ANIMATION_PROCESS process)
{
  m_queuedProcess = process;
}

