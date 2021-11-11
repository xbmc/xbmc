/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIScalarFeatureButton.h"

#include "guilib/LocalizeStrings.h"

#include <string>

using namespace KODI;
using namespace GAME;

CGUIScalarFeatureButton::CGUIScalarFeatureButton(const CGUIButtonControl& buttonTemplate,
                                                 IConfigurationWizard* wizard,
                                                 const CPhysicalFeature& feature,
                                                 unsigned int index)
  : CGUIFeatureButton(buttonTemplate, wizard, feature, index)
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
      const std::string& strPrompt = g_localizeStrings.Get(35090); // "Press %s"
      const std::string& strWarn = g_localizeStrings.Get(35091); // "Press %s (%d)"

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
