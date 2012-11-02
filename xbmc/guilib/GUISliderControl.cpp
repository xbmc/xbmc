/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "GUISliderControl.h"
#include "GUIInfoManager.h"
#include "Key.h"
#include "utils/MathUtils.h"
#include "GUIWindowManager.h"

static const SliderAction actions[] = {
  {"seek",    "PlayerControl(SeekPercentage(%2d))", PLAYER_PROGRESS, false},
  {"volume",  "SetVolume(%2d)",                     PLAYER_VOLUME,   true}
 };

CGUISliderControl::CGUISliderControl(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& backGroundTexture, const CTextureInfo& nibTexture, const CTextureInfo& nibTextureFocus, int iType)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_guiBackground(posX, posY, width, height, backGroundTexture)
    , m_guiSelectorLower(posX, posY, width, height, nibTexture)
    , m_guiSelectorUpper(posX, posY, width, height, nibTexture)
    , m_guiSelectorLowerFocus(posX, posY, width, height, nibTextureFocus)
    , m_guiSelectorUpperFocus(posX, posY, width, height, nibTextureFocus)
{
  m_iType = iType;
  m_rangeSelection = false;
  m_currentSelector = RangeSelectorLower; // use lower selector by default
  m_percentValues[0] = 0;
  m_percentValues[1] = 100;
  m_iStart = 0;
  m_iEnd = 100;
  m_iInterval = 1;
  m_fStart = 0.0f;
  m_fEnd = 1.0f;
  m_fInterval = 0.1f;
  m_intValues[0] = m_iStart;
  m_intValues[1] = m_iEnd;
  m_floatValues[0] = m_fStart;
  m_floatValues[1] = m_fEnd;
  ControlType = GUICONTROL_SLIDER;
  m_iInfoCode = 0;
  m_dragging = false;
  m_action = NULL;
}

CGUISliderControl::~CGUISliderControl(void)
{
}

void CGUISliderControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  bool dirty = false;

  dirty |= m_guiBackground.SetPosition( m_posX, m_posY );
  int infoCode = m_iInfoCode;
  if (m_action && (!m_dragging || m_action->fireOnDrag))
    infoCode = m_action->infoCode;
  if (infoCode)
  {
    int val;
    if (g_infoManager.GetInt(val, infoCode))
      SetIntValue(val);
  }

  float fScaleY = m_height == 0 ? 1.0f : m_height / m_guiBackground.GetTextureHeight();

  dirty |= m_guiBackground.SetHeight(m_height);
  dirty |= m_guiBackground.SetWidth(m_width);
  dirty |= m_guiBackground.Process(currentTime);

  CGUITexture &nibLower = (m_bHasFocus && !IsDisabled() && m_currentSelector == RangeSelectorLower) ? m_guiSelectorLowerFocus : m_guiSelectorLower;
  dirty |= ProcessSelector(nibLower, currentTime, fScaleY, RangeSelectorLower);
  if (m_rangeSelection)
  {
    CGUITexture &nibUpper = (m_bHasFocus && !IsDisabled() && m_currentSelector == RangeSelectorUpper) ? m_guiSelectorUpperFocus : m_guiSelectorUpper;
    dirty |= ProcessSelector(nibUpper, currentTime, fScaleY, RangeSelectorUpper);
  }

  if (dirty)
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

bool CGUISliderControl::ProcessSelector(CGUITexture &nib, unsigned int currentTime, float fScaleY, RangeSelector selector)
{
  bool dirty = false;
  // we render the nib centered at the appropriate percentage, except where the nib
  // would overflow the background image
  dirty |= nib.SetHeight(nib.GetTextureHeight() * fScaleY);
  dirty |= nib.SetWidth(nib.GetHeight() * 2);
  CAspectRatio ratio(CAspectRatio::AR_KEEP);
  ratio.align = ASPECT_ALIGN_LEFT | ASPECT_ALIGNY_CENTER;
  dirty |= nib.SetAspectRatio(ratio);
  CRect rect = nib.GetRenderRect();

  float offset = GetProportion(selector) * m_width - rect.Width() / 2;
  if (offset > m_width - rect.Width())
    offset = m_width - rect.Width();
  if (offset < 0)
    offset = 0;
  dirty |= nib.SetPosition(m_guiBackground.GetXPosition() + offset, m_guiBackground.GetYPosition());
  dirty |= nib.Process(currentTime);

  return dirty;
}

void CGUISliderControl::Render()
{
  m_guiBackground.Render();
  CGUITexture &nibLower = (m_bHasFocus && !IsDisabled() && m_currentSelector == RangeSelectorLower) ? m_guiSelectorLowerFocus : m_guiSelectorLower;
  nibLower.Render();
  if (m_rangeSelection)
  {
    CGUITexture &nibUpper = (m_bHasFocus && !IsDisabled() && m_currentSelector == RangeSelectorUpper) ? m_guiSelectorUpperFocus : m_guiSelectorUpper;
    nibUpper.Render();
  }
  CGUIControl::Render();
}

bool CGUISliderControl::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    switch (message.GetMessage())
    {
    case GUI_MSG_ITEM_SELECT:
      SetPercentage( message.GetParam1() );
      return true;
      break;

    case GUI_MSG_LABEL_RESET:
      {
        SetPercentage(0, RangeSelectorLower);
        SetPercentage(100, RangeSelectorUpper);
        return true;
      }
      break;
    }
  }

  return CGUIControl::OnMessage(message);
}

bool CGUISliderControl::OnAction(const CAction &action)
{
  switch ( action.GetID() )
  {
  case ACTION_MOVE_LEFT:
    //case ACTION_OSD_SHOW_VALUE_MIN:
    Move(-1);
    return true;

  case ACTION_MOVE_RIGHT:
    //case ACTION_OSD_SHOW_VALUE_PLUS:
    Move(1);
    return true;

  case ACTION_SELECT_ITEM:
    // switch between the two sliders
    SwitchRangeSelector();
    return true;

  default:
    break;
  }

  return CGUIControl::OnAction(action);
}

void CGUISliderControl::Move(int iNumSteps)
{
  bool rangeSwap = false;
  switch (m_iType)
  {
  case SPIN_CONTROL_TYPE_FLOAT:
    {
      float &value = m_floatValues[m_currentSelector];
      value += m_fInterval * iNumSteps;
      if (value < m_fStart) value = m_fStart;
      if (value > m_fEnd) value = m_fEnd;
      if (m_floatValues[0] > m_floatValues[1])
      {
        float valueLower = m_floatValues[0];
        m_floatValues[0] = m_floatValues[1];
        m_floatValues[1] = valueLower;
        rangeSwap = true;
      }
      break;
    }

  case SPIN_CONTROL_TYPE_INT:
    {
      int &value = m_intValues[m_currentSelector];
      value += m_iInterval * iNumSteps;
      if (value < m_iStart) value = m_iStart;
      if (value > m_iEnd) value = m_iEnd;
      if (m_intValues[0] > m_intValues[1])
      {
        int valueLower = m_intValues[0];
        m_intValues[0] = m_intValues[1];
        m_intValues[1] = valueLower;
        rangeSwap = true;
      }
      break;
    }

  default:
    {
      int &value = m_percentValues[m_currentSelector];
      value += m_iInterval * iNumSteps;
      if (value < 0) value = 0;
      if (value > 100) value = 100;
      if (m_percentValues[0] > m_percentValues[1])
      {
        int valueLower = m_percentValues[0];
        m_percentValues[0] = m_percentValues[1];
        m_percentValues[1] = valueLower;
        rangeSwap = true;
      }
      break;
    }
  }

  if (rangeSwap)
    SwitchRangeSelector();

  SendClick();
}

void CGUISliderControl::SendClick()
{
  int percent = MathUtils::round_int(100*GetProportion());
  SEND_CLICK_MESSAGE(GetID(), GetParentID(), percent);
  if (m_action && (!m_dragging || m_action->fireOnDrag))
  {
    CStdString action;
    action.Format(m_action->formatString, percent);
    CGUIMessage message(GUI_MSG_EXECUTE, m_controlID, m_parentID);
    message.SetStringParam(action);
    g_windowManager.SendMessage(message);    
  }
}

void CGUISliderControl::SetRangeSelection(bool rangeSelection)
{
  if (m_rangeSelection == rangeSelection)
    return;

  m_rangeSelection = rangeSelection;
  SetRangeSelector(RangeSelectorLower);
  SetInvalid();
}

void CGUISliderControl::SetRangeSelector(RangeSelector selector)
{
  if (m_currentSelector == selector)
    return;

  m_currentSelector = selector;
  SetInvalid();
}

void CGUISliderControl::SwitchRangeSelector()
{
  if (m_currentSelector == RangeSelectorLower)
    SetRangeSelector(RangeSelectorUpper);
  else
    SetRangeSelector(RangeSelectorLower);
}

void CGUISliderControl::SetPercentage(int iPercent, RangeSelector selector /* = RangeSelectorLower */, bool updateCurrent /* = false */)
{
  if (iPercent > 100) iPercent = 100;
  else if (iPercent < 0) iPercent = 0;

  int iPercentLower = selector == RangeSelectorLower ? iPercent : m_percentValues[0];
  int iPercentUpper = selector == RangeSelectorUpper ? iPercent : m_percentValues[1];

  if (!m_rangeSelection || iPercentLower <= iPercentUpper)
  {
    m_percentValues[0] = iPercentLower;
    m_percentValues[1] = iPercentUpper;
    if (updateCurrent)
      m_currentSelector = selector;
  }
  else
  {
    m_percentValues[0] = iPercentUpper;
    m_percentValues[1] = iPercentLower;
    if (updateCurrent)
        m_currentSelector = (selector == RangeSelectorLower ? RangeSelectorUpper : RangeSelectorLower);
  }
}

int CGUISliderControl::GetPercentage(RangeSelector selector /* = RangeSelectorLower */) const
{
  return m_percentValues[selector];
}

void CGUISliderControl::SetIntValue(int iValue, RangeSelector selector /* = RangeSelectorLower */, bool updateCurrent /* = false */)
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
    SetFloatValue((float)iValue);
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
  {
    if (iValue > m_iEnd) iValue = m_iEnd;
    else if (iValue < m_iStart) iValue = m_iStart;

    int iValueLower = selector == RangeSelectorLower ? iValue : m_intValues[0];
    int iValueUpper = selector == RangeSelectorUpper ? iValue : m_intValues[1];

    if (!m_rangeSelection || iValueLower <= iValueUpper)
    {
      m_intValues[0] = iValueLower;
      m_intValues[1] = iValueUpper;
      if (updateCurrent)
        m_currentSelector = selector;
    }
    else
    {
      m_intValues[0] = iValueUpper;
      m_intValues[1] = iValueLower;
      if (updateCurrent)
        m_currentSelector = (selector == RangeSelectorLower ? RangeSelectorUpper : RangeSelectorLower);
    }
  }
  else
    SetPercentage(iValue);
}

int CGUISliderControl::GetIntValue(RangeSelector selector /* = RangeSelectorLower */) const
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
    return (int)m_floatValues[selector];
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
    return m_intValues[selector];
  else
    return m_percentValues[selector];
}

void CGUISliderControl::SetFloatValue(float fValue, RangeSelector selector /* = RangeSelectorLower */, bool updateCurrent /* = false */)
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
  {
    if (fValue > m_fEnd) fValue = m_fEnd;
    else if (fValue < m_fStart) fValue = m_fStart;

    float fValueLower = selector == RangeSelectorLower ? fValue : m_floatValues[0];
    float fValueUpper = selector == RangeSelectorUpper ? fValue : m_floatValues[1];

    if (!m_rangeSelection || fValueLower <= fValueUpper)
    {
      m_floatValues[0] = fValueLower;
      m_floatValues[1] = fValueUpper;
      if (updateCurrent)
        m_currentSelector = selector;
    }
    else
    {
      m_floatValues[0] = fValueUpper;
      m_floatValues[1] = fValueLower;
      if (updateCurrent)
        m_currentSelector = (selector == RangeSelectorLower ? RangeSelectorUpper : RangeSelectorLower);
    }
  }
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
    SetIntValue((int)fValue);
  else
    SetPercentage((int)fValue);
}

float CGUISliderControl::GetFloatValue(RangeSelector selector /* = RangeSelectorLower */) const
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
    return m_floatValues[selector];
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
    return (float)m_intValues[selector];
  else
    return (float)m_percentValues[selector];
}

void CGUISliderControl::SetIntInterval(int iInterval)
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
    m_fInterval = (float)iInterval;
  else
    m_iInterval = iInterval;
}

void CGUISliderControl::SetFloatInterval(float fInterval)
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
    m_fInterval = fInterval;
  else
    m_iInterval = (int)fInterval;
}

void CGUISliderControl::SetRange(int iStart, int iEnd)
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
    SetFloatRange((float)iStart,(float)iEnd);
  else
  {
    m_intValues[0] = m_iStart = iStart;
    m_intValues[1] = m_iEnd = iEnd;
  }
}

void CGUISliderControl::SetFloatRange(float fStart, float fEnd)
{
  if (m_iType == SPIN_CONTROL_TYPE_INT)
    SetRange((int)fStart, (int)fEnd);
  else
  {
    m_floatValues[0] = m_fStart = fStart;
    m_floatValues[1] = m_fEnd = fEnd;
  }
}

void CGUISliderControl::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  m_guiBackground.FreeResources(immediately);
  m_guiSelectorLower.FreeResources(immediately);
  m_guiSelectorUpper.FreeResources(immediately);
  m_guiSelectorLowerFocus.FreeResources(immediately);
  m_guiSelectorUpperFocus.FreeResources(immediately);
}

void CGUISliderControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_guiBackground.DynamicResourceAlloc(bOnOff);
  m_guiSelectorLower.DynamicResourceAlloc(bOnOff);
  m_guiSelectorUpper.DynamicResourceAlloc(bOnOff);
  m_guiSelectorLowerFocus.DynamicResourceAlloc(bOnOff);
  m_guiSelectorUpperFocus.DynamicResourceAlloc(bOnOff);
}

void CGUISliderControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_guiBackground.AllocResources();
  m_guiSelectorLower.AllocResources();
  m_guiSelectorUpper.AllocResources();
  m_guiSelectorLowerFocus.AllocResources();
  m_guiSelectorUpperFocus.AllocResources();
}

void CGUISliderControl::SetInvalid()
{
  CGUIControl::SetInvalid();
  m_guiBackground.SetInvalid();
  m_guiSelectorLower.SetInvalid();
  m_guiSelectorUpper.SetInvalid();
  m_guiSelectorLowerFocus.SetInvalid();
  m_guiSelectorUpperFocus.SetInvalid();
}

bool CGUISliderControl::HitTest(const CPoint &point) const
{
  if (m_guiBackground.HitTest(point)) return true;
  if (m_guiSelectorLower.HitTest(point)) return true;
  if (m_rangeSelection && m_guiSelectorUpper.HitTest(point)) return true;
  return false;
}

void CGUISliderControl::SetFromPosition(const CPoint &point, bool guessSelector /* = false */)
{
  float fPercent = (point.x - m_guiBackground.GetXPosition()) / m_guiBackground.GetWidth();
  if (fPercent < 0) fPercent = 0;
  if (fPercent > 1) fPercent = 1;

  if (m_rangeSelection && guessSelector)
  {
    // choose selector which value is closer to value calculated from position
    if (fabs(GetPercentage(RangeSelectorLower) - 100 * fPercent) <= fabs(GetPercentage(RangeSelectorUpper) - 100 * fPercent))
      m_currentSelector = RangeSelectorLower;
    else
      m_currentSelector = RangeSelectorUpper;
  }

  switch (m_iType)
  {
  case SPIN_CONTROL_TYPE_FLOAT:
    {
      float fValue = m_fStart + (m_fEnd - m_fStart) * fPercent;
      SetFloatValue(fValue, m_currentSelector, true);
      break;
    }

  case SPIN_CONTROL_TYPE_INT:
    {
      int iValue = (int)(m_iStart + (float)(m_iEnd - m_iStart) * fPercent + 0.49f);
      SetIntValue(iValue, m_currentSelector, true);
      break;
    }

  default:
    {
      int iValue = (int)(fPercent * 100 + 0.49f);
      SetPercentage(iValue, m_currentSelector, true);
      break;
    }
  }
  SendClick();
}

EVENT_RESULT CGUISliderControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  m_dragging = false;
  if (event.m_id == ACTION_MOUSE_DRAG)
  {
    m_dragging = true;
    bool guessSelector = false;
    if (event.m_state == 1)
    { // grab exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, GetID(), GetParentID());
      SendWindowMessage(msg);
      guessSelector = true;
    }
    else if (event.m_state == 3)
    { // release exclusive access
      m_dragging = false;
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, 0, GetParentID());
      SendWindowMessage(msg);
    }
    SetFromPosition(point, guessSelector);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_MOUSE_LEFT_CLICK && m_guiBackground.HitTest(point))
  {
    SetFromPosition(point, true);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_MOUSE_WHEEL_UP)
  {
    Move(10);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_MOUSE_WHEEL_DOWN)
  {
    Move(-10);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_NOTIFY)
  {
    return EVENT_RESULT_PAN_HORIZONTAL_WITHOUT_INERTIA;
  }  
  else if (event.m_id == ACTION_GESTURE_BEGIN)
  { // grab exclusive access
    CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, GetID(), GetParentID());
    SendWindowMessage(msg);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_PAN)
  { // do the drag 
    SetFromPosition(point);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_END)
  { // release exclusive access
    CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, 0, GetParentID());
    SendWindowMessage(msg);
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

void CGUISliderControl::SetInfo(int iInfo)
{
  m_iInfoCode = iInfo;
}

CStdString CGUISliderControl::GetDescription() const
{
  if (!m_textValue.IsEmpty())
    return m_textValue;
  CStdString description;
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
  {
    if (m_rangeSelection)
      description.Format("[%2.2f, %2.2f]", m_floatValues[0], m_floatValues[1]);
    else
      description.Format("%2.2f", m_floatValues[0]);
  }
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
  {
    if (m_rangeSelection)
      description.Format("[%i, %i]", m_intValues[0], m_intValues[1]);
    else
      description.Format("%i", m_intValues[0]);
  }
  else
  {
    if (m_rangeSelection)
      description.Format("[%i%%, %i%%]", m_percentValues[0], m_percentValues[1]);
    else
      description.Format("%i%%", m_percentValues[0]);
  }
  return description;
}

bool CGUISliderControl::UpdateColors()
{
  bool changed = CGUIControl::UpdateColors();
  changed |= m_guiBackground.SetDiffuseColor(m_diffuseColor);
  changed |= m_guiSelectorLower.SetDiffuseColor(m_diffuseColor);
  changed |= m_guiSelectorUpper.SetDiffuseColor(m_diffuseColor);
  changed |= m_guiSelectorLowerFocus.SetDiffuseColor(m_diffuseColor);
  changed |= m_guiSelectorUpperFocus.SetDiffuseColor(m_diffuseColor);

  return changed;
}

float CGUISliderControl::GetProportion(RangeSelector selector /* = RangeSelectorLower */) const
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
    return (GetFloatValue(selector) - m_fStart) / (m_fEnd - m_fStart);
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
    return (float)(GetIntValue(selector) - m_iStart) / (float)(m_iEnd - m_iStart);
  return 0.01f * GetPercentage(selector);
}

void CGUISliderControl::SetAction(const CStdString &action)
{
  for (size_t i = 0; i < sizeof(actions)/sizeof(SliderAction); i++)
  {
    if (action.CompareNoCase(actions[i].action) == 0)
    {
      m_action = &actions[i];
      return;
    }
  }
  m_action = NULL;
}
