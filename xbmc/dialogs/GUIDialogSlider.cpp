/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSlider.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUISliderControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"

#define CONTROL_HEADING 10
#define CONTROL_SLIDER  11
#define CONTROL_LABEL   12

CGUIDialogSlider::CGUIDialogSlider(void)
    : CGUIDialog(WINDOW_DIALOG_SLIDER, "DialogSlider.xml")
{
  m_callback = NULL;
  m_callbackData = NULL;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogSlider::~CGUIDialogSlider(void) = default;

bool CGUIDialogSlider::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SELECT_ITEM)
  {
    Close();
    return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogSlider::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    if (message.GetSenderId() == CONTROL_SLIDER)
    {
      CGUISliderControl *slider = dynamic_cast<CGUISliderControl *>(GetControl(CONTROL_SLIDER));
      if (slider && m_callback)
      {
        m_callback->OnSliderChange(m_callbackData, slider);
        SET_CONTROL_LABEL(CONTROL_LABEL, slider->GetDescription());
      }
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    m_callback = NULL;
    m_callbackData = NULL;
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogSlider::SetSlider(const std::string &label, float value, float min, float delta, float max, ISliderCallback *callback, void *callbackData)
{
  SET_CONTROL_LABEL(CONTROL_HEADING, label);
  CGUISliderControl *slider = dynamic_cast<CGUISliderControl *>(GetControl(CONTROL_SLIDER));
  m_callback = callback;
  m_callbackData = callbackData;
  if (slider)
  {
    slider->SetType(SLIDER_CONTROL_TYPE_FLOAT);
    slider->SetFloatRange(min, max);
    slider->SetFloatInterval(delta);
    slider->SetFloatValue(value);
    if (m_callback)
    {
      m_callback->OnSliderChange(m_callbackData, slider);
      SET_CONTROL_LABEL(CONTROL_LABEL, slider->GetDescription());
    }
  }
}

void CGUIDialogSlider::OnWindowLoaded()
{
  // ensure our callbacks are NULL, incase we were loaded via some non-standard means
  m_callback = NULL;
  m_callbackData = NULL;
  CGUIDialog::OnWindowLoaded();
}

void CGUIDialogSlider::SetModalityType(DialogModalityType type)
{
  m_modalityType = type;
}

void CGUIDialogSlider::ShowAndGetInput(const std::string &label, float value, float min, float delta, float max, ISliderCallback *callback, void *callbackData)
{
  // grab the slider dialog
  CGUIDialogSlider *slider = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSlider>(WINDOW_DIALOG_SLIDER);
  if (!slider)
    return;

  // set the label and value
  slider->Initialize();
  slider->SetSlider(label, value, min, delta, max, callback, callbackData);
  slider->SetModalityType(DialogModalityType::MODAL);
  slider->Open();
}

void CGUIDialogSlider::Display(int label, float value, float min, float delta, float max, ISliderCallback *callback)
{
  // grab the slider dialog
  CGUIDialogSlider *slider = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSlider>(WINDOW_DIALOG_SLIDER);
  if (!slider)
    return;

  // set the label and value
  slider->Initialize();
  slider->SetAutoClose(1000);
  slider->SetSlider(g_localizeStrings.Get(label), value, min, delta, max, callback, NULL);
  slider->SetModalityType(DialogModalityType::MODELESS);
  slider->Open();
}
