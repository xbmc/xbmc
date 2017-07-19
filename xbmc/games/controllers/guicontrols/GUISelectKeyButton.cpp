/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUISelectKeyButton.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

#include <string>

using namespace KODI;
using namespace GAME;

CGUISelectKeyButton::CGUISelectKeyButton(const CGUIButtonControl& buttonTemplate,
                                         IConfigurationWizard* wizard,
                                         unsigned int index) :
  CGUIFeatureButton(buttonTemplate, wizard, GetFeature(), index)
{
}

const CControllerFeature& CGUISelectKeyButton::Feature(void) const
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
      std::string strPrompt = g_localizeStrings.Get(35169);  // "Press a key"
      std::string strWarn = g_localizeStrings.Get(35170);  // "Press a key ({1:d})"

      bInterrupted = DoPrompt(strPrompt, strWarn, "", waitEvent);

      m_state = GetNextState(m_state);

      break;
    }
    case STATE::NEED_INPUT:
    {
      std::string strPrompt = g_localizeStrings.Get(35090);  // "Press {0:s}"
      std::string strWarn = g_localizeStrings.Get(35091);  // "Press {0:s} ({1:d})"

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

void CGUISelectKeyButton::SetKey(const CControllerFeature &key)
{
  m_selectedKey = key;
}

void CGUISelectKeyButton::Reset(void)
{
  m_state = STATE::NEED_KEY;
  m_selectedKey.Reset();
}

CControllerFeature CGUISelectKeyButton::GetFeature()
{
  return CControllerFeature(35168); // "Select key"
}
