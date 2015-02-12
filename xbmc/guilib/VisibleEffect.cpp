/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "VisibleEffect.h"
#include "GUIInfoManager.h"
#include "utils/log.h"
#include "addons/Skin.h" // for the effect time adjustments
#include "utils/StringUtils.h"
#include "Tween.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

using namespace std;

CAnimEffect::CAnimEffect(const TiXmlElement *node, EFFECT_TYPE effect)
{
  m_effect = effect;
  // defaults
  m_delay = m_length = 0;
  m_pTweener.reset();
  // time and delay

  float temp;
  if (TIXML_SUCCESS == node->QueryFloatAttribute("time", &temp)) m_length = (unsigned int)(temp * g_SkinInfo->GetEffectsSlowdown());
  if (TIXML_SUCCESS == node->QueryFloatAttribute("delay", &temp)) m_delay = (unsigned int)(temp * g_SkinInfo->GetEffectsSlowdown());

  m_pTweener = GetTweener(node);
}

CAnimEffect::CAnimEffect(unsigned int delay, unsigned int length, EFFECT_TYPE effect)
{
  m_delay = delay;
  m_length = length;
  m_effect = effect;
  m_pTweener = std::shared_ptr<Tweener>(new LinearTweener());
}

CAnimEffect::~CAnimEffect()
{
}

CAnimEffect::CAnimEffect(const CAnimEffect &src)
{
  m_pTweener.reset();
  *this = src;
}

CAnimEffect& CAnimEffect::operator=(const CAnimEffect &src)
{
  if (&src == this) return *this;

  m_matrix = src.m_matrix;
  m_effect = src.m_effect;
  m_length = src.m_length;
  m_delay = src.m_delay;

  m_pTweener = src.m_pTweener;
  return *this;
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

std::shared_ptr<Tweener> CAnimEffect::GetTweener(const TiXmlElement *pAnimationNode)
{
  std::shared_ptr<Tweener> m_pTweener;
  const char *tween = pAnimationNode->Attribute("tween");
  if (tween)
  {
    if (strcmpi(tween, "linear")==0)
      m_pTweener = std::shared_ptr<Tweener>(new LinearTweener());
    else if (strcmpi(tween, "quadratic")==0)
      m_pTweener = std::shared_ptr<Tweener>(new QuadTweener());
    else if (strcmpi(tween, "cubic")==0)
      m_pTweener = std::shared_ptr<Tweener>(new CubicTweener());
    else if (strcmpi(tween, "sine")==0)
      m_pTweener = std::shared_ptr<Tweener>(new SineTweener());
    else if (strcmpi(tween, "back")==0)
      m_pTweener = std::shared_ptr<Tweener>(new BackTweener());
    else if (strcmpi(tween, "circle")==0)
      m_pTweener = std::shared_ptr<Tweener>(new CircleTweener());
    else if (strcmpi(tween, "bounce")==0)
      m_pTweener = std::shared_ptr<Tweener>(new BounceTweener());
    else if (strcmpi(tween, "elastic")==0)
      m_pTweener = std::shared_ptr<Tweener>(new ElasticTweener());

    const char *easing = pAnimationNode->Attribute("easing");
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

  float accel = 0;
  pAnimationNode->QueryFloatAttribute("acceleration", &accel);

  if (!m_pTweener)
  { // no tweener is specified - use a linear tweener
    // or quadratic if we have acceleration
    if (accel)
    {
      m_pTweener = std::shared_ptr<Tweener>(new QuadTweener(accel));
      m_pTweener->SetEasing(EASE_IN);
    }
    else
      m_pTweener = std::shared_ptr<Tweener>(new LinearTweener());
  }

  return m_pTweener;
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
  node->QueryFloatAttribute("start", &m_startAlpha);
  node->QueryFloatAttribute("end", &m_endAlpha);
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
    vector<string> commaSeparated = StringUtils::Split(startPos, ",");
    if (commaSeparated.size() > 1)
      m_startY = (float)atof(commaSeparated[1].c_str());
    if (!commaSeparated.empty())
      m_startX = (float)atof(commaSeparated[0].c_str());
  }
  const char *endPos = node->Attribute("end");
  if (endPos)
  {
    vector<string> commaSeparated = StringUtils::Split(endPos, ",");
    if (commaSeparated.size() > 1)
      m_endY = (float)atof(commaSeparated[1].c_str());
    if (!commaSeparated.empty())
      m_endX = (float)atof(commaSeparated[0].c_str());
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
  node->QueryFloatAttribute("start", &m_startAngle);
  node->QueryFloatAttribute("end", &m_endAngle);

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
      vector<string> commaSeparated = StringUtils::Split(centerPos, ",");
      if (commaSeparated.size() > 1)
        m_center.y = (float)atof(commaSeparated[1].c_str());
      if (!commaSeparated.empty())
        m_center.x = (float)atof(commaSeparated[0].c_str());
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

CZoomEffect::CZoomEffect(const TiXmlElement *node, const CRect &rect) : CAnimEffect(node, EFFECT_TYPE_ZOOM), m_center(CPoint(0,0))
{
  // effect defaults
  m_startX = m_startY = 100;
  m_endX = m_endY = 100;
  m_autoCenter = false;

  float startPosX = rect.x1;
  float startPosY = rect.y1;
  float endPosX = rect.x1;
  float endPosY = rect.y1;

  float width = max(rect.Width(), 0.001f);
  float height = max(rect.Height(),0.001f);

  const char *start = node->Attribute("start");
  if (start)
  {
    vector<string> params = StringUtils::Split(start, ",");
    if (params.size() == 1)
    {
      m_startX = (float)atof(params[0].c_str());
      m_startY = m_startX;
    }
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
      m_startX = (float)atof(params[2].c_str());
      m_startY = (float)atof(params[3].c_str());
      m_startX *= 100.0f / width;
      m_startY *= 100.0f / height;
    }
  }
  const char *end = node->Attribute("end");
  if (end)
  {
    vector<string> params = StringUtils::Split(end, ",");
    if (params.size() == 1)
    {
      m_endX = (float)atof(params[0].c_str());
      m_endY = m_endX;
    }
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
      m_endX = (float)atof(params[2].c_str());
      m_endY = (float)atof(params[3].c_str());
      m_endX *= 100.0f / width;
      m_endY *= 100.0f / height;
    }
  }
  const char *centerPos = node->Attribute("center");
  if (centerPos)
  {
    if (strcmpi(centerPos, "auto") == 0)
      m_autoCenter = true;
    else
    {
      vector<string> commaSeparated = StringUtils::Split(centerPos, ",");
      if (commaSeparated.size() > 1)
        m_center.y = (float)atof(commaSeparated[1].c_str());
      if (!commaSeparated.empty())
        m_center.x = (float)atof(commaSeparated[0].c_str());
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
  m_repeatAnim = ANIM_REPEAT_NONE;
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
    delete m_effects[i];
  m_effects.clear();
}

CAnimation &CAnimation::operator =(const CAnimation &src)
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
    delete m_effects[i];
  m_effects.clear();
  // and assign the others across
  for (unsigned int i = 0; i < src.m_effects.size(); i++)
  {
    CAnimEffect *newEffect = NULL;
    if (src.m_effects[i]->GetType() == CAnimEffect::EFFECT_TYPE_FADE)
      newEffect = new CFadeEffect(*(CFadeEffect *)src.m_effects[i]);
    else if (src.m_effects[i]->GetType() == CAnimEffect::EFFECT_TYPE_ZOOM)
      newEffect = new CZoomEffect(*(CZoomEffect *)src.m_effects[i]);
    else if (src.m_effects[i]->GetType() == CAnimEffect::EFFECT_TYPE_SLIDE)
      newEffect = new CSlideEffect(*(CSlideEffect *)src.m_effects[i]);
    else if (src.m_effects[i]->GetType() == CAnimEffect::EFFECT_TYPE_ROTATE_X ||
             src.m_effects[i]->GetType() == CAnimEffect::EFFECT_TYPE_ROTATE_Y ||
             src.m_effects[i]->GetType() == CAnimEffect::EFFECT_TYPE_ROTATE_Z)
      newEffect = new CRotateEffect(*(CRotateEffect *)src.m_effects[i]);
    if (newEffect)
      m_effects.push_back(newEffect);
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
  { // set normal process, so that Calculate() knows that we're finishing for zero length effects
    // it will be reset in RenderAnimation()
    m_currentProcess = ANIM_PROCESS_NORMAL;
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
      effect->Calculate(m_delay + m_amount, center);
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

CAnimation CAnimation::CreateFader(float start, float end, unsigned int delay, unsigned int length, ANIMATION_TYPE type)
{
  CAnimation anim;
  anim.m_type = type;
  anim.m_delay = delay;
  anim.m_length = length;
  anim.m_effects.push_back(new CFadeEffect(start, end, delay, length));
  return anim;
}

bool CAnimation::CheckCondition()
{
  return !m_condition || m_condition->Get();
}

void CAnimation::UpdateCondition(const CGUIListItem *item)
{
  if (!m_condition)
    return;
  bool condition = m_condition->Get(item);
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

void CAnimation::SetInitialCondition()
{
  m_lastCondition = m_condition ? m_condition->Get() : false;
  if (m_lastCondition)
    ApplyAnimation();
  else
    ResetAnimation();
}

void CAnimation::Create(const TiXmlElement *node, const CRect &rect, int context)
{
  if (!node || !node->FirstChild())
    return;

  // conditions and reversibility
  const char *condition = node->Attribute("condition");
  if (condition)
    m_condition = g_infoManager.Register(condition, context);
  const char *reverse = node->Attribute("reversible");
  if (reverse && strcmpi(reverse, "false") == 0)
    m_reversible = false;

  const TiXmlElement *effect = node->FirstChildElement("effect");

  std::string type = node->FirstChild()->Value();
  m_type = ANIM_TYPE_CONDITIONAL;
  if (effect) // new layout
    type = XMLUtils::GetAttribute(node, "type");

  if (StringUtils::StartsWithNoCase(type, "visible")) m_type = ANIM_TYPE_VISIBLE;
  else if (StringUtils::EqualsNoCase(type, "hidden")) m_type = ANIM_TYPE_HIDDEN;
  else if (StringUtils::EqualsNoCase(type, "focus"))  m_type = ANIM_TYPE_FOCUS;
  else if (StringUtils::EqualsNoCase(type, "unfocus"))  m_type = ANIM_TYPE_UNFOCUS;
  else if (StringUtils::EqualsNoCase(type, "windowopen"))  m_type = ANIM_TYPE_WINDOW_OPEN;
  else if (StringUtils::EqualsNoCase(type, "windowclose"))  m_type = ANIM_TYPE_WINDOW_CLOSE;
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
    std::string type = XMLUtils::GetAttribute(node, "effect");
    AddEffect(type, node, rect);
  }
  while (effect)
  { // new layout:
    // <animation type="focus" condition="blahdiblah" reversible="false">
    //   <effect type="fade" start="0" end="100" delay="10" time="2000" />
    //   ...
    // </animation>
    std::string type = XMLUtils::GetAttribute(effect, "type");
    AddEffect(type, effect, rect);
    effect = effect->NextSiblingElement("effect");
  }
  // compute the minimum delay and maximum length
  m_delay = 0xffffffff;
  unsigned int total = 0;
  for (vector<CAnimEffect*>::const_iterator i = m_effects.begin(); i != m_effects.end(); ++i)
  {
    m_delay = min(m_delay, (*i)->GetDelay());
    total   = max(total, (*i)->GetLength());
  }
  m_length = total - m_delay;
}

void CAnimation::AddEffect(const std::string &type, const TiXmlElement *node, const CRect &rect)
{
  CAnimEffect *effect = NULL;
  if (StringUtils::EqualsNoCase(type, "fade"))
    effect = new CFadeEffect(node, m_type < 0);
  else if (StringUtils::EqualsNoCase(type, "slide"))
    effect = new CSlideEffect(node);
  else if (StringUtils::EqualsNoCase(type, "rotate"))
    effect = new CRotateEffect(node, CAnimEffect::EFFECT_TYPE_ROTATE_Z);
  else if (StringUtils::EqualsNoCase(type, "rotatey"))
    effect = new CRotateEffect(node, CAnimEffect::EFFECT_TYPE_ROTATE_Y);
  else if (StringUtils::EqualsNoCase(type, "rotatex"))
    effect = new CRotateEffect(node, CAnimEffect::EFFECT_TYPE_ROTATE_X);
  else if (StringUtils::EqualsNoCase(type, "zoom"))
    effect = new CZoomEffect(node, rect);

  if (effect)
    m_effects.push_back(effect);
}

CScroller::CScroller(unsigned int duration /* = 200 */, std::shared_ptr<Tweener> tweener /* = NULL */)
{
  m_scrollValue = 0;
  m_delta = 0;
  m_startTime = 0;
  m_startPosition = 0;
  m_hasResumePoint = false;
  m_lastTime = 0;
  m_duration = duration > 0 ? duration : 1;
  m_pTweener = tweener;
}

CScroller::CScroller(const CScroller& right)
{
  m_pTweener.reset();
  *this = right;
}

CScroller& CScroller::operator=(const CScroller &right)
{
  if (&right == this) return *this;

  m_scrollValue = right.m_scrollValue;
  m_delta = right.m_delta;
  m_startTime = right.m_startTime;
  m_startPosition = right.m_startPosition;
  m_hasResumePoint = right.m_hasResumePoint;
  m_lastTime = right.m_lastTime;
  m_duration = right.m_duration;
  m_pTweener = right.m_pTweener;
  return *this;
}

CScroller::~CScroller()
{
}

void CScroller::ScrollTo(float endPos)
{
  float delta = endPos - m_scrollValue;
    // if there is scrolling running in same direction - set resume point
  m_hasResumePoint = m_delta != 0 && delta * m_delta > 0 && m_pTweener ? m_pTweener->HasResumePoint() : 0;

  m_delta = delta;
  m_startPosition = m_scrollValue;
  m_startTime = m_lastTime;
}

float CScroller::Tween(float progress)
{
  if (m_pTweener)
  {
    if (m_hasResumePoint) // tweener with in_and_out easing
    {
      // time linear transformation (y = a*x + b): 0 -> resumePoint and 1 -> 1
      // resumePoint = a * 0 + b and 1 = a * 1 + b
      // a = 1 - resumePoint , b = resumePoint
      // our resume point is 0.5
      // a = 0.5 , b = 0.5
      progress = 0.5f * progress + 0.5f;
      // tweener value linear transformation (y = a*x + b): resumePointValue -> 0 and 1 -> 1
      // 0 = a * resumePointValue and 1 = a * 1 + b
      // a = 1 / ( 1 - resumePointValue) , b = -resumePointValue / (1 - resumePointValue)
      // we assume resumePointValue = Tween(resumePoint) = Tween(0.5) = 0.5
      // (this is true for tweener with in_and_out easing - it's rotationally symmetric about (0.5,0.5) continuous function)
      // a = 2 , b = -1
      return (2 * m_pTweener->Tween(progress, 0, 1, 1) - 1);
    }
    else 
      return m_pTweener->Tween(progress, 0, 1, 1);
  }
  else
    return progress;
}

bool CScroller::Update(unsigned int time)
{
  m_lastTime = time;
  if (m_delta != 0)
  {
    if (time - m_startTime >= m_duration) // we are finished
    {
      m_scrollValue = m_startPosition + m_delta;
      m_startTime = 0;
      m_hasResumePoint = false;
      m_delta = 0;
      m_startPosition = 0;
    }
    else
      m_scrollValue = m_startPosition + Tween((float)(time - m_startTime) / m_duration) * m_delta;
    return true;
  }
  else
    return false;
}
