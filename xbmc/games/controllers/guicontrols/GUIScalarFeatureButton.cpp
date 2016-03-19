/*
 *      Copyright (C) 2016 Team Kodi
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

#include "GUIScalarFeatureButton.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

#include <string>

using namespace GAME;

CGUIScalarFeatureButton::CGUIScalarFeatureButton(const CGUIButtonControl& buttonTemplate,
                                                 IConfigurationWizard* wizard,
                                                 const CControllerFeature& feature,
                                                 unsigned int index) :
  CGUIFeatureButton(buttonTemplate, wizard, feature, index)
{
  Reset();
}

bool CGUIScalarFeatureButton::PromptForInput(CEvent& waitEvent)
{
  bool bInterrupted = false;

  switch (m_state)
  {
    case STATE::NEED_INPUT:
    {
      std::string strPrompt = g_localizeStrings.Get(35090);  // "Press %s"
      std::string strWarn = g_localizeStrings.Get(35091);  // "Press %s (%d)"

      bInterrupted = DoPrompt(strPrompt, strWarn, m_feature.Label(), waitEvent);

      m_state = GetNextState(m_state);

      break;
    }
    default:
      break;
  }

  return bInterrupted;
}

bool CGUIScalarFeatureButton::IsFinished(void) const
{
  return m_state >= STATE::FINISHED;
}

void CGUIScalarFeatureButton::Reset(void)
{
  m_state = STATE::NEED_INPUT;
}
