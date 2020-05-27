/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIFeatureButton.h"
#include "games/controllers/ControllerFeature.h"

namespace KODI
{
namespace GAME
{
class CGUISelectKeyButton : public CGUIFeatureButton
{
public:
  CGUISelectKeyButton(const CGUIButtonControl& buttonTemplate,
                      IConfigurationWizard* wizard,
                      unsigned int index);

  ~CGUISelectKeyButton() override = default;

  // implementation of IFeatureButton
  const CControllerFeature& Feature(void) const override;
  bool AllowWizard() const override { return false; }
  bool PromptForInput(CEvent& waitEvent) override;
  bool IsFinished() const override;
  bool NeedsKey() const override { return m_state == STATE::NEED_KEY; }
  void SetKey(const CControllerFeature& key) override;
  void Reset() override;

private:
  static CControllerFeature GetFeature();

  enum class STATE
  {
    NEED_KEY,
    NEED_INPUT,
    FINISHED,
  };

  STATE m_state = STATE::NEED_KEY;

  CControllerFeature m_selectedKey;
};
} // namespace GAME
} // namespace KODI
