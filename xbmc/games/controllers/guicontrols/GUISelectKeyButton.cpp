/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUISelectKeyButton.h"

#include "guilib/LocalizeStrings.h"

#include <string>

using namespace KODI;
using namespace GAME;

CGUISelectKeyButton::CGUISelectKeyButton(const CGUIButtonControl& buttonTemplate,
                                         IConfigurationWizard* wizard,
                                         unsigned int index)
  : CGUIFeatureButton(buttonTemplate, wizard, GetFeature(), index)
{
}

const CPhysicalFeature& CGUISelectKeyButton::Feature(void) const
{
  if (m_state == STATE::NEED_INPUT)
    return m_selectedKey;

  return CGUIFeatureButton::Feature();
}

bool CGUISelectKeyButton::PromptForInput(CEvent& waitEvent)
{
  bool bInterrupted = false;

  switch (m_state)
  {
    case STATE::NEED_KEY:
    {
      const std::string& strPrompt = g_localizeStrings.Get(35169); // "Press a key"
      const std::string& strWarn = g_localizeStrings.Get(35170); // "Press a key ({1:d})"

      bInterrupted = DoPrompt(strPrompt, strWarn, "", waitEvent);

      m_state = GetNextState(m_state);

      break;
    }
    case STATE::NEED_INPUT:
    {
      const std::string& strPrompt = g_localizeStrings.Get(35090); // "Press {0:s}"
      const std::string& strWarn = g_localizeStrings.Get(35091); // "Press {0:s} ({1:d})"

      bInterrupted = DoPrompt(strPrompt, strWarn, m_selectedKey.Label(), waitEvent);

      m_state = GetNextState(m_state);

      break;
    }
    default:
      break;
  }

  return bInterrupted;
}

bool CGUISelectKeyButton::IsFinished(void) const
{
  return m_state >= STATE::FINISHED;
}

void CGUISelectKeyButton::SetKey(const CPhysicalFeature& key)
{
  m_selectedKey = key;
}

void CGUISelectKeyButton::Reset(void)
{
  m_state = STATE::NEED_KEY;
  m_selectedKey.Reset();
}

CPhysicalFeature CGUISelectKeyButton::GetFeature()
{
  return CPhysicalFeature(35168); // "Select key"
}
