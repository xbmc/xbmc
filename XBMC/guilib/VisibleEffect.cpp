#include "include.h"
#include "VisibleEffect.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "SkinInfo.h" // for the effect time adjustments
#include "guiImage.h" // for FRECT
#include "Tween.h"

CAnimation::CAnimation()
{
  m_pTweener = NULL;
  Reset();
}

CAnimation::CAnimation(const CAnimation& src)
{
  m_pTweener = NULL;
  *this=src;
}

CAnimation &CAnimation::operator=(const CAnimation &src)
{
  m_amount = src.m_amount;
  m_type = src.m_type;
  m_effect = src.m_effect;
  m_currentState = src.m_currentState;
  m_currentProcess = src.m_currentProcess;
  m_queuedProcess = src.m_queuedProcess;
 
  m_startX = src.m_startX;
  m_startY = src.m_startY;
  m_endX   = src.m_endX;
  m_endY = src.m_endY;
  m_centerX = src.m_centerX;
  m_centerY = src.m_centerY;
  m_startAlpha = src.m_startAlpha;
  m_endAlpha=src.m_endAlpha;

  // timing variables
  m_start = src.m_start;
  m_length = src.m_length;
  m_delay = src.m_delay;
  m_repeatAnim = src.m_repeatAnim;
  m_reversible = src.m_reversible;    

  m_condition = src.m_condition;
  m_lastCondition = src.m_lastCondition; 

  m_matrix = src.m_matrix; //has operator=

  if (m_pTweener)
    m_pTweener->Free();

  m_pTweener = src.m_pTweener;
  if (m_pTweener)
    m_pTweener->IncRef();

  return *this;
}

CAnimation::~CAnimation()
{
  if (m_pTweener) 
    m_pTweener->Free();
  m_pTweener = NULL;
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
  m_condition = 0;
  m_reversible = true;
  m_lastCondition = false;
  m_repeatAnim = ANIM_REPEAT_NONE;
  if (m_pTweener)
    m_pTweener->Free();
  m_pTweener = NULL;
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
    m_effect = EFFECT_TYPE_ROTATE_Z;
  else if (strcmpi(effect, "rotatey") == 0)
    m_effect = EFFECT_TYPE_ROTATE_Y;
  else if (strcmpi(effect, "rotatex") == 0)
    m_effect = EFFECT_TYPE_ROTATE_X;
  else if (strcmpi(effect, "zoom") == 0)
    m_effect = EFFECT_TYPE_ZOOM;
  // time and delay
  node->Attribute("time", (int *)&m_length);
  node->Attribute("delay", (int *)&m_delay);
  m_length = (unsigned int)(m_length * g_SkinInfo.GetEffectsSlowdown());
  m_delay = (unsigned int)(m_delay * g_SkinInfo.GetEffectsSlowdown());

  if (m_pTweener)
  {
    m_pTweener->Free();
    m_pTweener = NULL;
  }
  const char *tween = node->Attribute("tween");
  if (tween)
  {
    if (strcmpi(tween, "linear")==0)
      m_pTweener = new LinearTweener();
    else if (strcmpi(tween, "quadratic")==0)
      m_pTweener = new QuadTweener();
    else if (strcmpi(tween, "cubic")==0)
      m_pTweener = new CubicTweener();
    else if (strcmpi(tween, "sine")==0)
      m_pTweener = new SineTweener();
    else if (strcmpi(tween, "back")==0)
      m_pTweener = new BackTweener();
    else if (strcmpi(tween, "circle")==0)
      m_pTweener = new CircleTweener();
    else if (strcmpi(tween, "bounce")==0)
      m_pTweener = new BounceTweener();
    else if (strcmpi(tween, "elastic")==0)
      m_pTweener = new ElasticTweener();
    
    const char *easing = node->Attribute("easing");
    if (m_pTweener && easing)
    {
      if (strcmpi(easing, "in")==0)
        m_pTweener->SetEasing(EASE_IN);
      else if (strcmpi(easing, "out")==0)
        m_pTweener->SetEasing(EASE_OUT);
      else if (strcmpi(easing, "inout")==0)
        m_pTweener->SetEasing(EASE_INOUT);
    }
  }

  double accel;
  node->Attribute("acceleration", &accel);

  if (!m_pTweener)
  { // no tweener is specified - use a linear tweener
    // or quadratic if we have acceleration
    if (accel)
    {
      m_pTweener = new QuadTweener((float)accel);
      m_pTweener->SetEasing(EASE_IN);
    }
    else
      m_pTweener = new LinearTweener();
  }

  // reversible (defaults to true)
  const char *reverse = node->Attribute("reversible");
  if (reverse && strcmpi(reverse, "false") == 0)
    m_reversible = false;

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
  else if (m_effect >= EFFECT_TYPE_ROTATE_X && m_effect <= EFFECT_TYPE_ROTATE_Z)
  {
    double temp;
    if (node->Attribute("start", &temp)) m_startX = (float)temp;
    if (node->Attribute("end", &temp)) m_endX = (float)temp;

    // convert to a negative to account for our reversed Y axis (Needed for X and Z ???)
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
  float offset = m_amount;
  if (m_pTweener)
    offset = m_pTweener->Tween(m_amount, 0.0f, 1.0f, 1.0f);
  if (m_effect == EFFECT_TYPE_FADE)
  {
    m_matrix.SetFader(((float)(m_endAlpha - m_startAlpha) * offset + m_startAlpha) * 0.01f);
  }
  else if (m_effect == EFFECT_TYPE_SLIDE)
  {
    m_matrix.SetTranslation((m_endX - m_startX)*offset + m_startX, (m_endY - m_startY)*offset + m_startY, 0);
  }
  else if (m_effect == EFFECT_TYPE_ROTATE_X)
  { 
    m_matrix.SetXRotation(((m_endX - m_startX)*offset + m_startX) * DEGREE_TO_RADIAN, m_centerX, m_centerY, 1.0f);
  }
  else if (m_effect == EFFECT_TYPE_ROTATE_Y)
  {
    m_matrix.SetYRotation(((m_endX - m_startX)*offset + m_startX) * DEGREE_TO_RADIAN, m_centerX, m_centerY, 1.0f);
  }
  else if (m_effect == EFFECT_TYPE_ROTATE_Z)
  { // note coordinate aspect ratio is not generally square in the XY plane, so correct for it.
    m_matrix.SetZRotation(((m_endX - m_startX)*offset + m_startX) * DEGREE_TO_RADIAN, m_centerX, m_centerY, g_graphicsContext.GetScalingPixelRatio());
  }
  else if (m_effect == EFFECT_TYPE_ZOOM)
  {
    float scaleX = ((m_endX - m_startX)*offset + m_startX) * 0.01f;
    float scaleY = ((m_endY - m_startY)*offset + m_startY) * 0.01f;
    m_matrix.SetScaler(scaleX, scaleY, m_centerX, m_centerY);
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
  m_queuedProcess = ANIM_PROCESS_NONE;
  if (m_repeatAnim == ANIM_REPEAT_PULSE)
  { // pulsed anims auto-reverse
    m_amount = 1.0f;
    m_currentProcess = ANIM_PROCESS_REVERSE;
    m_currentState = ANIM_STATE_IN_PROCESS;
  }
  else if (m_repeatAnim == ANIM_REPEAT_LOOP)
  { // looped anims start over
    m_amount = 0.0f;
    m_currentProcess = ANIM_PROCESS_NORMAL;
    m_currentState = ANIM_STATE_IN_PROCESS;
  }
  else
  {
    m_currentProcess = ANIM_PROCESS_NONE;
    m_currentState = ANIM_STATE_APPLIED;
    m_amount = 1.0f;
  }
  Calculate();
}

void CAnimation::UpdateCondition(DWORD contextWindow)
{
  bool condition = g_infoManager.GetBool(m_condition, contextWindow);
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

void CAnimation::SetInitialCondition(DWORD contextWindow)
{
  m_lastCondition = g_infoManager.GetBool(m_condition, contextWindow);
  if (m_lastCondition)
    ApplyAnimation();
  else
    ResetAnimation();
}

void CAnimation::QueueAnimation(ANIMATION_PROCESS process)
{
  m_queuedProcess = process;
}