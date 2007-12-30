#include "include.h"
#include "VisibleEffect.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "SkinInfo.h" // for the effect time adjustments
#include "guiImage.h" // for FRECT
#include "Tween.h"

CAnimEffect::CAnimEffect(const TiXmlElement *node, EFFECT_TYPE effect)
{
  m_effect = effect;
  // defaults
  m_delay = m_length = 0;
  m_pTweener = NULL;
  // time and delay
  float temp;
  if (g_SkinInfo.ResolveConstant(node->Attribute("time"), temp)) m_length = (unsigned int)(temp * g_SkinInfo.GetEffectsSlowdown());
  if (g_SkinInfo.ResolveConstant(node->Attribute("delay"), temp)) m_delay = (unsigned int)(temp * g_SkinInfo.GetEffectsSlowdown());

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

  float accel;
  g_SkinInfo.ResolveConstant(node->Attribute("acceleration"), accel);

  if (!m_pTweener)
  { // no tweener is specified - use a linear tweener
    // or quadratic if we have acceleration
    if (accel)
    {
      m_pTweener = new QuadTweener(accel);
      m_pTweener->SetEasing(EASE_IN);
    }
    else
      m_pTweener = new LinearTweener();
  }
  m_referenceCount = 1;
}

CAnimEffect::CAnimEffect(unsigned int delay, unsigned int length, EFFECT_TYPE effect)
{
  m_delay = delay;
  m_length = length;
  m_effect = effect;
  m_pTweener = new LinearTweener();
  m_referenceCount = 1;
}

CAnimEffect::~CAnimEffect()
{
  assert(m_referenceCount == 0);
  if (m_pTweener) 
    delete m_pTweener;
}

void CAnimEffect::AddReference()
{
  m_referenceCount++;
}

void CAnimEffect::Free()
{
  assert(m_referenceCount);
  m_referenceCount--;
  if (!m_referenceCount)
    delete this;
}

void CAnimEffect::Calculate(unsigned int time, const CPoint &center)
{
  assert(m_delay + m_length);
  // calculate offset and tweening
  float offset = 0.0f;  // delayed forward, or finished reverse
  if (time >= m_delay && time < m_delay + m_length)
    offset = (float)(time - m_delay) / m_length;
  else if (time >= m_delay + m_length)
    offset = 1.0f;
  if (m_pTweener)
    offset = m_pTweener->Tween(offset, 0.0f, 1.0f, 1.0f);
  // and apply the effect
  ApplyEffect(offset, center);
}

void CAnimEffect::ApplyState(ANIMATION_STATE state, const CPoint &center)
{
  float offset = (state == ANIM_STATE_APPLIED) ? 1.0f : 0.0f;
  ApplyEffect(offset, center);
}

CFadeEffect::CFadeEffect(const TiXmlElement *node, bool reverseDefaults) : CAnimEffect(node, EFFECT_TYPE_FADE)
{
  if (reverseDefaults)
  { // out effect defaults
    m_startAlpha = 100.0f;
    m_endAlpha = 0;
  }
  else
  { // in effect defaults
    m_startAlpha = 0;
    m_endAlpha = 100.0f;
  }
  if (node->Attribute("start")) g_SkinInfo.ResolveConstant(node->Attribute("start"), m_startAlpha);
  if (node->Attribute("end")) g_SkinInfo.ResolveConstant(node->Attribute("end"), m_endAlpha);
  if (m_startAlpha > 100.0f) m_startAlpha = 100.0f;
  if (m_endAlpha > 100.0f) m_endAlpha = 100.0f;
  if (m_startAlpha < 0) m_startAlpha = 0;
  if (m_endAlpha < 0) m_endAlpha = 0;
}

CFadeEffect::CFadeEffect(float start, float end, unsigned int delay, unsigned int length) : CAnimEffect(delay, length, EFFECT_TYPE_FADE)
{
  m_startAlpha = start;
  m_endAlpha = end;
}

void CFadeEffect::ApplyEffect(float offset, const CPoint &center)
{
  m_matrix.SetFader(((m_endAlpha - m_startAlpha) * offset + m_startAlpha) * 0.01f);
}

CSlideEffect::CSlideEffect(const TiXmlElement *node) : CAnimEffect(node, EFFECT_TYPE_SLIDE)
{
  m_startX = m_endX = 0;
  m_startY = m_endY = 0;
  const char *startPos = node->Attribute("start");
  if (startPos)
  {
    vector<CStdString> commaSeparated;
    StringUtils::SplitString(startPos, ",", commaSeparated);
    if (commaSeparated.size() > 1)
      g_SkinInfo.ResolveConstant(commaSeparated[1], m_startY);
    g_SkinInfo.ResolveConstant(commaSeparated[0], m_startX);
  }
  const char *endPos = node->Attribute("end");
  if (endPos)
  {
    vector<CStdString> commaSeparated;
    StringUtils::SplitString(endPos, ",", commaSeparated);
    if (commaSeparated.size() > 1)
      g_SkinInfo.ResolveConstant(commaSeparated[1], m_endY);
    g_SkinInfo.ResolveConstant(commaSeparated[0], m_endX);
  }
}

void CSlideEffect::ApplyEffect(float offset, const CPoint &center)
{
  m_matrix.SetTranslation((m_endX - m_startX)*offset + m_startX, (m_endY - m_startY)*offset + m_startY, 0);
}

CRotateEffect::CRotateEffect(const TiXmlElement *node, EFFECT_TYPE effect) : CAnimEffect(node, effect)
{
  m_startAngle = m_endAngle = 0;
  m_autoCenter = false;
  if (node->Attribute("start")) g_SkinInfo.ResolveConstant(node->Attribute("start"), m_startAngle);
  if (node->Attribute("end")) g_SkinInfo.ResolveConstant(node->Attribute("end"), m_endAngle);

  // convert to a negative to account for our reversed Y axis (Needed for X and Z ???)
  m_startAngle *= -1;
  m_endAngle *= -1;

  const char *centerPos = node->Attribute("center");
  if (centerPos)
  {
    if (strcmpi(centerPos, "auto") == 0)
      m_autoCenter = true;
    else
    {
      vector<CStdString> commaSeparated;
      StringUtils::SplitString(centerPos, ",", commaSeparated);
      if (commaSeparated.size() > 1)
        g_SkinInfo.ResolveConstant(commaSeparated[1], m_center.y);
      g_SkinInfo.ResolveConstant(commaSeparated[0], m_center.x);
    }
  }
}

void CRotateEffect::ApplyEffect(float offset, const CPoint &center)
{
  static const float degree_to_radian = 0.01745329252f;
  if (m_autoCenter)
    m_center = center;
  if (m_effect == EFFECT_TYPE_ROTATE_X)
    m_matrix.SetXRotation(((m_endAngle - m_startAngle)*offset + m_startAngle) * degree_to_radian, m_center.x, m_center.y, 1.0f);
  else if (m_effect == EFFECT_TYPE_ROTATE_Y)
    m_matrix.SetYRotation(((m_endAngle - m_startAngle)*offset + m_startAngle) * degree_to_radian, m_center.x, m_center.y, 1.0f);
  else if (m_effect == EFFECT_TYPE_ROTATE_Z) // note coordinate aspect ratio is not generally square in the XY plane, so correct for it.
    m_matrix.SetZRotation(((m_endAngle - m_startAngle)*offset + m_startAngle) * degree_to_radian, m_center.x, m_center.y, g_graphicsContext.GetScalingPixelRatio());
}

CZoomEffect::CZoomEffect(const TiXmlElement *node, const FRECT &rect) : CAnimEffect(node, EFFECT_TYPE_ZOOM)
{
  // effect defaults
  m_startX = m_startY = 100;
  m_endX = m_endY = 100;
  m_center = CPoint(0,0);
  m_autoCenter = false;

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
    {
      g_SkinInfo.ResolveConstant(params[0], m_startX);
      m_startY = m_startX;
    }
    else if (params.size() == 2)
    {
      g_SkinInfo.ResolveConstant(params[0], m_startX);
      g_SkinInfo.ResolveConstant(params[1], m_startY);
    }
    else if (params.size() == 4)
    { // format is start="x,y,width,height"
      // use width and height from our rect to calculate our sizing
      g_SkinInfo.ResolveConstant(params[0], startPosX);
      g_SkinInfo.ResolveConstant(params[1], startPosY);
      g_SkinInfo.ResolveConstant(params[2], m_startX);
      g_SkinInfo.ResolveConstant(params[3], m_startY);
      m_startX *= 100.0f / rect.right;
      m_startY *= 100.0f / rect.bottom;
    }
  }
  const char *end = node->Attribute("end");
  if (end)
  {
    CStdStringArray params;
    StringUtils::SplitString(end, ",", params);
    if (params.size() == 1)
    {
      g_SkinInfo.ResolveConstant(params[0], m_endX);
      m_endY = m_endX;
    }
    else if (params.size() == 2)
    {
      g_SkinInfo.ResolveConstant(params[0], m_endX);
      g_SkinInfo.ResolveConstant(params[1], m_endY);
    }
    else if (params.size() == 4)
    { // format is start="x,y,width,height"
      // use width and height from our rect to calculate our sizing
      g_SkinInfo.ResolveConstant(params[0], endPosX);
      g_SkinInfo.ResolveConstant(params[1], endPosY);
      g_SkinInfo.ResolveConstant(params[2], m_endX);
      g_SkinInfo.ResolveConstant(params[3], m_endY);
      m_endX *= 100.0f / rect.right;
      m_endY *= 100.0f / rect.bottom;
    }
  }
  const char *centerPos = node->Attribute("center");
  if (centerPos)
  {
    if (strcmpi(centerPos, "auto") == 0)
      m_autoCenter = true;
    else
    {
      vector<CStdString> commaSeparated;
      StringUtils::SplitString(centerPos, ",", commaSeparated);
      if (commaSeparated.size() > 1)
        g_SkinInfo.ResolveConstant(commaSeparated[1], m_center.y);
      g_SkinInfo.ResolveConstant(commaSeparated[0], m_center.x);
    }
  }
  else
  { // no center specified
    // calculate the center position...
    if (m_startX)
    {
      float scale = m_endX / m_startX;
      if (scale != 1)
        m_center.x = (endPosX - scale*startPosX) / (1 - scale);
    }
    if (m_startY)
    {
      float scale = m_endY / m_startY;
      if (scale != 1)
        m_center.y = (endPosY - scale*startPosY) / (1 - scale);
    }
  }
}

void CZoomEffect::ApplyEffect(float offset, const CPoint &center)
{
  if (m_autoCenter)
    m_center = center;
  float scaleX = ((m_endX - m_startX)*offset + m_startX) * 0.01f;
  float scaleY = ((m_endY - m_startY)*offset + m_startY) * 0.01f;
  m_matrix.SetScaler(scaleX, scaleY, m_center.x, m_center.y);
}

CAnimation::CAnimation()
{
  m_type = ANIM_TYPE_NONE;
  m_reversible = true;
  m_condition = 0;
  m_currentState = ANIM_STATE_NONE;
  m_currentProcess = ANIM_PROCESS_NONE;
  m_queuedProcess = ANIM_PROCESS_NONE;
  m_lastCondition = false;
  m_length = 0;
  m_delay = 0;
  m_start = 0;
  m_amount = 0;
}

CAnimation::CAnimation(const CAnimation &src)
{
  *this = src;
}

CAnimation::~CAnimation()
{
  for (unsigned int i = 0; i < m_effects.size(); i++)
    m_effects[i]->Free();
  m_effects.clear();
}

const CAnimation &CAnimation::operator =(const CAnimation &src)
{
  if (this == &src) return *this; // same
  m_type = src.m_type;
  m_reversible = src.m_reversible;
  m_condition = src.m_condition;
  m_repeatAnim = src.m_repeatAnim;
  m_lastCondition = src.m_lastCondition;
  m_queuedProcess = src.m_queuedProcess;
  m_currentProcess = src.m_currentProcess;
  m_currentState = src.m_currentState;
  m_start = src.m_start;
  m_length = src.m_length;
  m_delay = src.m_delay;
  m_amount = src.m_amount;
  // clear all our effects
  for (unsigned int i = 0; i < m_effects.size(); i++)
    m_effects[i]->Free();
  m_effects.clear();
  // and assign the others across
  for (unsigned int i = 0; i < src.m_effects.size(); i++)
  {
    CAnimEffect *effect = src.m_effects[i];
    m_effects.push_back(effect);
    effect->AddReference();
  }
  return *this;
}

void CAnimation::Animate(unsigned int time, bool startAnim)
{
  // First start any queued animations
  if (m_queuedProcess == ANIM_PROCESS_NORMAL)
  {
    if (m_currentProcess == ANIM_PROCESS_REVERSE)
      m_start = time - m_amount;  // reverse direction of animation
    else
      m_start = time;
    m_currentProcess = ANIM_PROCESS_NORMAL;
  }
  else if (m_queuedProcess == ANIM_PROCESS_REVERSE)
  {
    if (m_currentProcess == ANIM_PROCESS_NORMAL)
      m_start = time - (m_length - m_amount); // reverse direction of animation
    else if (m_currentProcess == ANIM_PROCESS_NONE)
      m_start = time;
    m_currentProcess = ANIM_PROCESS_REVERSE;
  }
  // reset the queued state once we've rendered to ensure allocation has occured
  if (startAnim || m_queuedProcess == ANIM_PROCESS_REVERSE)
    m_queuedProcess = ANIM_PROCESS_NONE;

  // Update our animation process
  if (m_currentProcess == ANIM_PROCESS_NORMAL)
  {
    if (time - m_start < m_delay)
    {
      m_amount = 0;
      m_currentState = ANIM_STATE_DELAYED;
    }
    else if (time - m_start < m_length + m_delay)
    {
      m_amount = time - m_start - m_delay;
      m_currentState = ANIM_STATE_IN_PROCESS;
    }
    else
    {
      m_amount = m_length;
      if (m_repeatAnim == ANIM_REPEAT_PULSE && m_lastCondition)
      { // pulsed anims auto-reverse
        m_currentProcess = ANIM_PROCESS_REVERSE;
        m_start = time;
      }
      else if (m_repeatAnim == ANIM_REPEAT_LOOP && m_lastCondition)
      { // looped anims start over
        m_amount = 0;
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
      m_amount = m_length - (time - m_start);
      m_currentState = ANIM_STATE_IN_PROCESS;
    }
    else
    {
      m_amount = 0;
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

void CAnimation::ResetAnimation()
{
  m_queuedProcess = ANIM_PROCESS_NONE;
  m_currentProcess = ANIM_PROCESS_NONE;
  m_currentState = ANIM_STATE_NONE;
}

void CAnimation::ApplyAnimation()
{
  m_queuedProcess = ANIM_PROCESS_NONE;
  if (m_repeatAnim == ANIM_REPEAT_PULSE)
  { // pulsed anims auto-reverse
    m_amount = m_length;
    m_currentProcess = ANIM_PROCESS_REVERSE;
    m_currentState = ANIM_STATE_IN_PROCESS;
  }
  else if (m_repeatAnim == ANIM_REPEAT_LOOP)
  { // looped anims start over
    m_amount = 0;
    m_currentProcess = ANIM_PROCESS_NORMAL;
    m_currentState = ANIM_STATE_IN_PROCESS;
  }
  else
  {
    m_currentProcess = ANIM_PROCESS_NONE;
    m_currentState = ANIM_STATE_APPLIED;
    m_amount = m_length;
  }
  Calculate(CPoint());
}

void CAnimation::Calculate(const CPoint &center)
{
  for (unsigned int i = 0; i < m_effects.size(); i++)
  {
    CAnimEffect *effect = m_effects[i];
    if (effect->GetLength())
      effect->Calculate(m_amount, center);
    else
    { // effect has length zero, so either apply complete
      if (m_currentProcess == ANIM_PROCESS_NORMAL)
        effect->ApplyState(ANIM_STATE_APPLIED, center);
      else
        effect->ApplyState(ANIM_STATE_NONE, center);
    }
  }
}

void CAnimation::RenderAnimation(TransformMatrix &matrix, const CPoint &center)
{
  if (m_currentProcess != ANIM_PROCESS_NONE)
    Calculate(center);
  // If we have finished an animation, reset the animation state
  // We do this here (rather than in Animate()) as we need the
  // currentProcess information in the UpdateStates() function of the
  // window and control classes.
  if (m_currentState == ANIM_STATE_APPLIED)
  {
    m_currentProcess = ANIM_PROCESS_NONE;
    m_queuedProcess = ANIM_PROCESS_NONE;
  }
  if (m_currentState != ANIM_STATE_NONE)
  {
    for (unsigned int i = 0; i < m_effects.size(); i++)
      matrix *= m_effects[i]->GetTransform();
  }
}

void CAnimation::QueueAnimation(ANIMATION_PROCESS process)
{
  m_queuedProcess = process;
}

CAnimation *CAnimation::CreateFader(float start, float end, unsigned int delay, unsigned int length)
{
  CAnimation *anim = new CAnimation();
  if (anim)
  {
    CFadeEffect *effect = new CFadeEffect(start, end, delay, length);
    if (effect)
      anim->AddEffect(effect);
  }
  return anim;
}

void CAnimation::UpdateCondition(DWORD contextWindow, const CGUIListItem *item)
{
  bool condition = g_infoManager.GetBool(m_condition, contextWindow, item);
  if (condition && !m_lastCondition)
    QueueAnimation(ANIM_PROCESS_NORMAL);
  else if (!condition && m_lastCondition)
  {
    if (m_reversible)
      QueueAnimation(ANIM_PROCESS_REVERSE);
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

void CAnimation::Create(const TiXmlElement *node, const FRECT &rect)
{
  if (!node || !node->FirstChild())
    return;

  // conditions and reversibility
  const char *condition = node->Attribute("condition");
  if (condition)
    m_condition = g_infoManager.TranslateString(condition);
  const char *reverse = node->Attribute("reversible");
  if (reverse && strcmpi(reverse, "false") == 0)
    m_reversible = false; 

  const TiXmlElement *effect = node->FirstChildElement("effect");

  CStdString type = node->FirstChild()->Value();
  m_type = ANIM_TYPE_CONDITIONAL;
  if (effect) // new layout
    type = node->Attribute("type");

  if (type.Left(7).Equals("visible")) m_type = ANIM_TYPE_VISIBLE;
  else if (type.Equals("hidden")) m_type = ANIM_TYPE_HIDDEN;
  else if (type.Equals("focus"))  m_type = ANIM_TYPE_FOCUS;
  else if (type.Equals("unfocus"))  m_type = ANIM_TYPE_UNFOCUS;
  else if (type.Equals("windowopen"))  m_type = ANIM_TYPE_WINDOW_OPEN;
  else if (type.Equals("windowclose"))  m_type = ANIM_TYPE_WINDOW_CLOSE;
  // sanity check
  if (m_type == ANIM_TYPE_CONDITIONAL)
  {
    if (!m_condition)
    {
      CLog::Log(LOGERROR, "Control has invalid animation type (no condition or no type)");
      return;
    }

    // pulsed or loop animations
    const char *pulse = node->Attribute("pulse");
    if (pulse && strcmpi(pulse, "true") == 0)
      m_repeatAnim = ANIM_REPEAT_PULSE;
    const char *loop = node->Attribute("loop");
    if (loop && strcmpi(loop, "true") == 0)
      m_repeatAnim = ANIM_REPEAT_LOOP;
  }

  if (!effect)
  { // old layout:
    // <animation effect="fade" start="0" end="100" delay="10" time="2000" condition="blahdiblah" reversible="false">focus</animation>
    CStdString type = node->Attribute("effect");
    AddEffect(type, node, rect);
  }
  while (effect)
  { // new layout:
    // <animation type="focus" condition="blahdiblah" reversible="false">
    //   <effect type="fade" start="0" end="100" delay="10" time="2000" />
    //   ...
    // </animation>
    CStdString type = effect->Attribute("type");
    AddEffect(type, effect, rect);
    effect = effect->NextSiblingElement("effect");
  }
}

void CAnimation::AddEffect(const CStdString &type, const TiXmlElement *node, const FRECT &rect)
{
  CAnimEffect *effect = NULL;
  if (type.Equals("fade"))
    effect = new CFadeEffect(node, m_type < 0);
  else if (type.Equals("slide"))
    effect = new CSlideEffect(node);
  else if (type.Equals("rotate"))
    effect = new CRotateEffect(node, CAnimEffect::EFFECT_TYPE_ROTATE_Z);
  else if (type.Equals("rotatey"))
    effect = new CRotateEffect(node, CAnimEffect::EFFECT_TYPE_ROTATE_Y);
  else if (type.Equals("rotatex"))
    effect = new CRotateEffect(node, CAnimEffect::EFFECT_TYPE_ROTATE_X);
  else if (type.Equals("zoom"))
    effect = new CZoomEffect(node, rect);

  if (effect)
    AddEffect(effect);
}

void CAnimation::AddEffect(CAnimEffect *effect)
{
  m_effects.push_back(effect);
  // our delay is the minimum of all the effect delays
  if (effect->GetDelay() < m_delay)
    m_delay = effect->GetDelay();
  // our length is the maximum of all the effect lengths
  if (effect->GetLength() > m_delay + m_length)
    m_length = effect->GetLength() - m_delay;
}