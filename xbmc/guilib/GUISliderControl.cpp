/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUISliderControl.h"

#include "GUIComponent.h"
#include "GUIInfoManager.h"
#include "GUIWindowManager.h"
#include "ServiceBroker.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseEvent.h"
#include "input/mouse/MouseStat.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"

using namespace KODI;

static const SliderAction actions[] = {
    {"seek", "PlayerControl(SeekPercentage({:2f}))", PLAYER_PROGRESS, false},
    {"pvr.seek", "PVR.SeekPercentage({:2f})", PVR_TIMESHIFT_PROGRESS_PLAY_POS, false},
    {"volume", "SetVolume({:2f})", PLAYER_VOLUME, true}};

CGUISliderControl::CGUISliderControl(int parentID,
                                     int controlID,
                                     float posX,
                                     float posY,
                                     float width,
                                     float height,
                                     const CTextureInfo& backGroundTexture,
                                     const CTextureInfo& backGroundTextureDisabled,
                                     const CTextureInfo& nibTexture,
                                     const CTextureInfo& nibTextureFocus,
                                     const CTextureInfo& nibTextureDisabled,
                                     int iType,
                                     ORIENTATION orientation)
  : CGUIControl(parentID, controlID, posX, posY, width, height),
    m_guiBackground(CGUITexture::CreateTexture(posX, posY, width, height, backGroundTexture)),
    m_guiBackgroundDisabled(
        CGUITexture::CreateTexture(posX, posY, width, height, backGroundTextureDisabled)),
    m_guiSelectorLower(CGUITexture::CreateTexture(posX, posY, width, height, nibTexture)),
    m_guiSelectorUpper(CGUITexture::CreateTexture(posX, posY, width, height, nibTexture)),
    m_guiSelectorLowerFocus(CGUITexture::CreateTexture(posX, posY, width, height, nibTextureFocus)),
    m_guiSelectorUpperFocus(CGUITexture::CreateTexture(posX, posY, width, height, nibTextureFocus)),
    m_guiSelectorLowerDisabled(
        CGUITexture::CreateTexture(posX, posY, width, height, nibTextureDisabled)),
    m_guiSelectorUpperDisabled(
        CGUITexture::CreateTexture(posX, posY, width, height, nibTextureDisabled))
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
  m_orientation = orientation;
  m_iInfoCode = 0;
  m_dragging = false;
  m_action = NULL;
}

CGUISliderControl::CGUISliderControl(const CGUISliderControl& control)
  : CGUIControl(control),
    m_guiBackground(control.m_guiBackground->Clone()),
    m_guiBackgroundDisabled(control.m_guiBackgroundDisabled->Clone()),
    m_guiSelectorLower(control.m_guiSelectorLower->Clone()),
    m_guiSelectorUpper(control.m_guiSelectorUpper->Clone()),
    m_guiSelectorLowerFocus(control.m_guiSelectorLowerFocus->Clone()),
    m_guiSelectorUpperFocus(control.m_guiSelectorUpperFocus->Clone()),
    m_guiSelectorLowerDisabled(control.m_guiSelectorLowerDisabled->Clone()),
    m_guiSelectorUpperDisabled(control.m_guiSelectorUpperDisabled->Clone()),
    m_iType(control.m_iType),
    m_rangeSelection(control.m_rangeSelection),
    m_currentSelector(control.m_currentSelector),
    m_percentValues(control.m_percentValues),
    m_intValues(control.m_intValues),
    m_iStart(control.m_iStart),
    m_iInterval(control.m_iInterval),
    m_iEnd(control.m_iEnd),
    m_floatValues(control.m_floatValues),
    m_fStart(control.m_fStart),
    m_fInterval(control.m_fInterval),
    m_fEnd(control.m_fEnd),
    m_iInfoCode(control.m_iInfoCode),
    m_textValue(control.m_textValue),
    m_action(control.m_action),
    m_dragging(control.m_dragging),
    m_orientation(control.m_orientation)
{
}

void CGUISliderControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  bool dirty = false;
  CGUITexture* guiBackground =
      !IsDisabled() ? m_guiBackground.get() : m_guiBackgroundDisabled.get();

  dirty |= guiBackground->SetPosition(m_posX, m_posY);
  int infoCode = m_iInfoCode;
  if (m_action && (!m_dragging || m_action->fireOnDrag))
    infoCode = m_action->infoCode;
  if (infoCode)
  {
    int val;
    if (CServiceBroker::GetGUI()->GetInfoManager().GetInt(val, infoCode, INFO::DEFAULT_CONTEXT))
      SetIntValue(val);
  }

  dirty |= guiBackground->SetHeight(m_height);
  dirty |= guiBackground->SetWidth(m_width);
  dirty |= guiBackground->Process(currentTime);

  CGUITexture* nibLower = nullptr;
  if (IsActive() && m_bHasFocus && !IsDisabled() && m_currentSelector == RangeSelectorLower)
    nibLower = m_guiSelectorLowerFocus.get();
  else if (!IsDisabled())
    nibLower = m_guiSelectorLower.get();
  else
    nibLower = m_guiSelectorLowerDisabled.get();

  float fScale = 1.0f;

  if (m_orientation == HORIZONTAL && guiBackground->GetTextureHeight() != 0)
    fScale = m_height / guiBackground->GetTextureHeight();
  else if (m_width != 0 && nibLower->GetTextureWidth() != 0)
    fScale = m_width / nibLower->GetTextureWidth();
  dirty |= ProcessSelector(guiBackground, nibLower, currentTime, fScale, RangeSelectorLower);
  if (m_rangeSelection)
  {
    CGUITexture* nibUpper = nullptr;
    if (IsActive() && m_bHasFocus && !IsDisabled() && m_currentSelector == RangeSelectorUpper)
      nibUpper = m_guiSelectorUpperFocus.get();
    else if (!IsDisabled())
      nibUpper = m_guiSelectorUpper.get();
    else
      nibUpper = m_guiSelectorUpperDisabled.get();

    if (m_orientation == HORIZONTAL && guiBackground->GetTextureHeight() != 0)
      fScale = m_height / guiBackground->GetTextureHeight();
    else if (m_width != 0 && nibUpper->GetTextureWidth() != 0)
      fScale = m_width / nibUpper->GetTextureWidth();

    dirty |= ProcessSelector(guiBackground, nibUpper, currentTime, fScale, RangeSelectorUpper);
  }

  if (dirty)
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

bool CGUISliderControl::ProcessSelector(CGUITexture* background,
                                        CGUITexture* nib,
                                        unsigned int currentTime,
                                        float fScale,
                                        RangeSelector selector)
{
  bool dirty = false;
  // we render the nib centered at the appropriate percentage, except where the nib
  // would overflow the background image
  if (m_orientation == HORIZONTAL)
  {
    dirty |= nib->SetHeight(nib->GetTextureHeight() * fScale);
    dirty |= nib->SetWidth(nib->GetHeight() * 2);
  }
  else
  {
    dirty |= nib->SetWidth(nib->GetTextureWidth() * fScale);
    dirty |= nib->SetHeight(nib->GetWidth() * 2);
  }
  CAspectRatio ratio(CAspectRatio::AR_KEEP);
  ratio.align = ASPECT_ALIGN_LEFT | ASPECT_ALIGNY_CENTER;
  dirty |= nib->SetAspectRatio(ratio);
  dirty |= nib->Process(currentTime);
  CRect rect = nib->GetRenderRect();

  float offset;
  if (m_orientation == HORIZONTAL)
  {
    offset = GetProportion(selector) * m_width - rect.Width() / 2;
    if (offset > m_width - rect.Width())
      offset = m_width - rect.Width();
    if (offset < 0)
      offset = 0;
    dirty |= nib->SetPosition(background->GetXPosition() + offset, background->GetYPosition());
  }
  else
  {
    offset = GetProportion(selector) * m_height - rect.Height() / 2;
    if (offset > m_height - rect.Height())
      offset = m_height - rect.Height();
    if (offset < 0)
      offset = 0;
    dirty |= nib->SetPosition(background->GetXPosition(),
                              background->GetYPosition() + background->GetHeight() - offset -
                                  ((nib->GetHeight() - rect.Height()) / 2 + rect.Height()));
  }
  dirty |= nib->Process(currentTime); // need to process again as the position may have changed

  return dirty;
}

void CGUISliderControl::Render()
{
  if (!IsDisabled())
    m_guiBackground->Render();
  else
    m_guiBackgroundDisabled->Render();

  CGUITexture* nibLower = nullptr;
  if (IsActive() && m_bHasFocus && !IsDisabled() && m_currentSelector == RangeSelectorLower)
    nibLower = m_guiSelectorLowerFocus.get();
  else if (!IsDisabled())
    nibLower = m_guiSelectorLower.get();
  else
    nibLower = m_guiSelectorLowerDisabled.get();
  nibLower->Render();
  if (m_rangeSelection)
  {
    CGUITexture* nibUpper = nullptr;
    if (IsActive() && m_bHasFocus && !IsDisabled() && m_currentSelector == RangeSelectorUpper)
      nibUpper = m_guiSelectorUpperFocus.get();
    else if (!IsDisabled())
      nibUpper = m_guiSelectorUpper.get();
    else
      nibUpper = m_guiSelectorUpperDisabled.get();
    nibUpper->Render();
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
      SetPercentage( (float)message.GetParam1() );
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
    if (IsActive() && m_orientation == HORIZONTAL)
    {
      Move(-1);
      return true;
    }
    break;

  case ACTION_MOVE_RIGHT:
    if (IsActive() && m_orientation == HORIZONTAL)
    {
      Move(1);
      return true;
    }
    break;

  case ACTION_MOVE_UP:
    if (IsActive() && m_orientation == VERTICAL)
    {
      Move(1);
      return true;
    }
    break;

  case ACTION_MOVE_DOWN:
    if (IsActive() && m_orientation == VERTICAL)
    {
      Move(-1);
      return true;
    }
    break;

  case ACTION_SELECT_ITEM:
    if (m_rangeSelection)
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
  case SLIDER_CONTROL_TYPE_FLOAT:
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

  case SLIDER_CONTROL_TYPE_INT:
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

  case SLIDER_CONTROL_TYPE_PERCENTAGE:
  default:
    {
      float &value = m_percentValues[m_currentSelector];
      value += m_iInterval * iNumSteps;
      if (value < 0) value = 0;
      if (value > 100) value = 100;
      if (m_percentValues[0] > m_percentValues[1])
      {
        float valueLower = m_percentValues[0];
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
  float percent = 100*GetProportion();
  SEND_CLICK_MESSAGE(GetID(), GetParentID(), MathUtils::round_int(static_cast<double>(percent)));
  if (m_action && (!m_dragging || m_action->fireOnDrag))
  {
    std::string action = StringUtils::Format(m_action->formatString, percent);
    CGUIMessage message(GUI_MSG_EXECUTE, m_controlID, m_parentID);
    message.SetStringParam(action);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);
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

void CGUISliderControl::SetPercentage(float percent, RangeSelector selector /* = RangeSelectorLower */, bool updateCurrent /* = false */)
{
  if (percent > 100) percent = 100;
  else if (percent < 0) percent = 0;

  float percentLower = selector == RangeSelectorLower ? percent : m_percentValues[0];
  float percentUpper = selector == RangeSelectorUpper ? percent : m_percentValues[1];
  const float oldValues[2] = {m_percentValues[0], m_percentValues[1]};

  if (!m_rangeSelection || percentLower <= percentUpper)
  {
    m_percentValues[0] = percentLower;
    m_percentValues[1] = percentUpper;
    if (updateCurrent)
      m_currentSelector = selector;
  }
  else
  {
    m_percentValues[0] = percentUpper;
    m_percentValues[1] = percentLower;
    if (updateCurrent)
        m_currentSelector = (selector == RangeSelectorLower ? RangeSelectorUpper : RangeSelectorLower);
  }
  if (oldValues[0] != m_percentValues[0] || oldValues[1] != m_percentValues[1])
    MarkDirtyRegion();
}

float CGUISliderControl::GetPercentage(RangeSelector selector /* = RangeSelectorLower */) const
{
  return m_percentValues[selector];
}

void CGUISliderControl::SetIntValue(int iValue, RangeSelector selector /* = RangeSelectorLower */, bool updateCurrent /* = false */)
{
  if (m_iType == SLIDER_CONTROL_TYPE_FLOAT)
    SetFloatValue((float)iValue, selector, updateCurrent);
  else if (m_iType == SLIDER_CONTROL_TYPE_INT)
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
    SetPercentage((float)iValue, selector, updateCurrent);
}

int CGUISliderControl::GetIntValue(RangeSelector selector /* = RangeSelectorLower */) const
{
  if (m_iType == SLIDER_CONTROL_TYPE_FLOAT)
    return (int)m_floatValues[selector];
  else if (m_iType == SLIDER_CONTROL_TYPE_INT)
    return m_intValues[selector];
  else
    return MathUtils::round_int(static_cast<double>(m_percentValues[selector]));
}

void CGUISliderControl::SetFloatValue(float fValue, RangeSelector selector /* = RangeSelectorLower */, bool updateCurrent /* = false */)
{
  if (m_iType == SLIDER_CONTROL_TYPE_FLOAT)
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
  else if (m_iType == SLIDER_CONTROL_TYPE_INT)
    SetIntValue((int)fValue, selector, updateCurrent);
  else
    SetPercentage(fValue, selector, updateCurrent);
}

float CGUISliderControl::GetFloatValue(RangeSelector selector /* = RangeSelectorLower */) const
{
  if (m_iType == SLIDER_CONTROL_TYPE_FLOAT)
    return m_floatValues[selector];
  else if (m_iType == SLIDER_CONTROL_TYPE_INT)
    return (float)m_intValues[selector];
  else
    return m_percentValues[selector];
}

void CGUISliderControl::SetIntInterval(int iInterval)
{
  if (m_iType == SLIDER_CONTROL_TYPE_FLOAT)
    m_fInterval = (float)iInterval;
  else
    m_iInterval = iInterval;
}

void CGUISliderControl::SetFloatInterval(float fInterval)
{
  if (m_iType == SLIDER_CONTROL_TYPE_FLOAT)
    m_fInterval = fInterval;
  else
    m_iInterval = (int)fInterval;
}

void CGUISliderControl::SetRange(int iStart, int iEnd)
{
  if (m_iType == SLIDER_CONTROL_TYPE_FLOAT)
    SetFloatRange((float)iStart,(float)iEnd);
  else
  {
    m_intValues[0] = m_iStart = iStart;
    m_intValues[1] = m_iEnd = iEnd;
  }
}

void CGUISliderControl::SetFloatRange(float fStart, float fEnd)
{
  if (m_iType == SLIDER_CONTROL_TYPE_INT)
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
  m_guiBackground->FreeResources(immediately);
  m_guiBackgroundDisabled->FreeResources(immediately);
  m_guiSelectorLower->FreeResources(immediately);
  m_guiSelectorUpper->FreeResources(immediately);
  m_guiSelectorLowerFocus->FreeResources(immediately);
  m_guiSelectorUpperFocus->FreeResources(immediately);
  m_guiSelectorLowerDisabled->FreeResources(immediately);
  m_guiSelectorUpperDisabled->FreeResources(immediately);
}

void CGUISliderControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_guiBackground->DynamicResourceAlloc(bOnOff);
  m_guiBackgroundDisabled->DynamicResourceAlloc(bOnOff);
  m_guiSelectorLower->DynamicResourceAlloc(bOnOff);
  m_guiSelectorUpper->DynamicResourceAlloc(bOnOff);
  m_guiSelectorLowerFocus->DynamicResourceAlloc(bOnOff);
  m_guiSelectorUpperFocus->DynamicResourceAlloc(bOnOff);
  m_guiSelectorLowerDisabled->DynamicResourceAlloc(bOnOff);
  m_guiSelectorUpperDisabled->DynamicResourceAlloc(bOnOff);
}

void CGUISliderControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_guiBackground->AllocResources();
  m_guiBackgroundDisabled->AllocResources();
  m_guiSelectorLower->AllocResources();
  m_guiSelectorUpper->AllocResources();
  m_guiSelectorLowerFocus->AllocResources();
  m_guiSelectorUpperFocus->AllocResources();
  m_guiSelectorLowerDisabled->AllocResources();
  m_guiSelectorUpperDisabled->AllocResources();
}

void CGUISliderControl::SetInvalid()
{
  CGUIControl::SetInvalid();
  m_guiBackground->SetInvalid();
  m_guiBackgroundDisabled->SetInvalid();
  m_guiSelectorLower->SetInvalid();
  m_guiSelectorUpper->SetInvalid();
  m_guiSelectorLowerFocus->SetInvalid();
  m_guiSelectorUpperFocus->SetInvalid();
  m_guiSelectorLowerDisabled->SetInvalid();
  m_guiSelectorUpperDisabled->SetInvalid();
}

bool CGUISliderControl::HitTest(const CPoint &point) const
{
  if (m_guiBackground->HitTest(point))
    return true;
  if (m_guiSelectorLower->HitTest(point))
    return true;
  if (m_rangeSelection && m_guiSelectorUpper->HitTest(point))
    return true;
  return false;
}

void CGUISliderControl::SetFromPosition(const CPoint &point, bool guessSelector /* = false */)
{

  float fPercent;
  if (m_orientation == HORIZONTAL)
    fPercent = (point.x - m_guiBackground->GetXPosition()) / m_guiBackground->GetWidth();
  else
    fPercent = (m_guiBackground->GetYPosition() + m_guiBackground->GetHeight() - point.y) /
               m_guiBackground->GetHeight();

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
    case SLIDER_CONTROL_TYPE_FLOAT:
    {
      float fValue = m_fStart + (m_fEnd - m_fStart) * fPercent;
      SetFloatValue(MathUtils::RoundF(fValue, m_fInterval), m_currentSelector, true);
      break;
    }

  case SLIDER_CONTROL_TYPE_INT:
    {
      int iValue = (int)(m_iStart + (float)(m_iEnd - m_iStart) * fPercent + 0.49f);
      SetIntValue(iValue, m_currentSelector, true);
      break;
    }

  case SLIDER_CONTROL_TYPE_PERCENTAGE:
  default:
    {
      SetPercentage(fPercent * 100, m_currentSelector, true);
      break;
    }
  }
  SendClick();
}

EVENT_RESULT CGUISliderControl::OnMouseEvent(const CPoint& point, const MOUSE::CMouseEvent& event)
{
  m_dragging = false;
  if (event.m_id == ACTION_MOUSE_DRAG || event.m_id == ACTION_MOUSE_DRAG_END)
  {
    m_dragging = true;
    bool guessSelector = false;
    if (static_cast<HoldAction>(event.m_state) == HoldAction::DRAG)
    { // grab exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, GetID(), GetParentID());
      SendWindowMessage(msg);
      guessSelector = true;
    }
    else if (static_cast<HoldAction>(event.m_state) == HoldAction::DRAG_END)
    { // release exclusive access
      m_dragging = false;
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, 0, GetParentID());
      SendWindowMessage(msg);
    }
    SetFromPosition(point, guessSelector);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_MOUSE_LEFT_CLICK && m_guiBackground->HitTest(point))
  {
    SetFromPosition(point, true);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_MOUSE_WHEEL_UP)
  {
    if (m_guiBackground->HitTest(point))
    {
      Move(10);
      return EVENT_RESULT_HANDLED;
    }
  }
  else if (event.m_id == ACTION_MOUSE_WHEEL_DOWN)
  {
    if (m_guiBackground->HitTest(point))
    {
      Move(-10);
      return EVENT_RESULT_HANDLED;
    }
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
  else if (event.m_id == ACTION_GESTURE_END || event.m_id == ACTION_GESTURE_ABORT)
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

std::string CGUISliderControl::GetDescription() const
{
  if (!m_textValue.empty())
    return m_textValue;
  std::string description;
  if (m_iType == SLIDER_CONTROL_TYPE_FLOAT)
  {
    if (m_rangeSelection)
      description = StringUtils::Format("[{:2.2f}, {:2.2f}]", m_floatValues[0], m_floatValues[1]);
    else
      description = StringUtils::Format("{:2.2f}", m_floatValues[0]);
  }
  else if (m_iType == SLIDER_CONTROL_TYPE_INT)
  {
    if (m_rangeSelection)
      description = StringUtils::Format("[{}, {}]", m_intValues[0], m_intValues[1]);
    else
      description = std::to_string(m_intValues[0]);
  }
  else
  {
    if (m_rangeSelection)
      description = StringUtils::Format("[{}%, {}%]", MathUtils::round_int(static_cast<double>(m_percentValues[0])),
                                        MathUtils::round_int(static_cast<double>(m_percentValues[1])));
    else
      description = StringUtils::Format("{}%", MathUtils::round_int(static_cast<double>(m_percentValues[0])));
  }
  return description;
}

bool CGUISliderControl::UpdateColors(const CGUIListItem* item)
{
  bool changed = CGUIControl::UpdateColors(nullptr);
  changed |= m_guiBackground->SetDiffuseColor(m_diffuseColor);
  changed |= m_guiBackgroundDisabled->SetDiffuseColor(m_diffuseColor);
  changed |= m_guiSelectorLower->SetDiffuseColor(m_diffuseColor);
  changed |= m_guiSelectorUpper->SetDiffuseColor(m_diffuseColor);
  changed |= m_guiSelectorLowerFocus->SetDiffuseColor(m_diffuseColor);
  changed |= m_guiSelectorUpperFocus->SetDiffuseColor(m_diffuseColor);
  changed |= m_guiSelectorLowerDisabled->SetDiffuseColor(m_diffuseColor);
  changed |= m_guiSelectorUpperDisabled->SetDiffuseColor(m_diffuseColor);

  return changed;
}

float CGUISliderControl::GetProportion(RangeSelector selector /* = RangeSelectorLower */) const
{
  if (m_iType == SLIDER_CONTROL_TYPE_FLOAT)
    return m_fStart != m_fEnd ? (GetFloatValue(selector) - m_fStart) / (m_fEnd - m_fStart) : 0.0f;
  else if (m_iType == SLIDER_CONTROL_TYPE_INT)
    return m_iStart != m_iEnd ? (float)(GetIntValue(selector) - m_iStart) / (float)(m_iEnd - m_iStart) : 0.0f;
  return 0.01f * GetPercentage(selector);
}

void CGUISliderControl::SetAction(const std::string &action)
{
  for (const SliderAction& a : actions)
  {
    if (StringUtils::EqualsNoCase(action, a.action))
    {
      m_action = &a;
      return;
    }
  }
  m_action = NULL;
}
