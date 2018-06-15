/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

namespace KODI
{
namespace RETRO
{
  class CGUIGameSettingsHandle;
}

namespace GAME
{
  class CDialogGameAdvancedSettings: public CGUIDialog
  {
  public:
    CDialogGameAdvancedSettings();
    ~CDialogGameAdvancedSettings() override = default;

    // implementation of CGUIControl via CGUIDialog
    bool OnMessage(CGUIMessage &message) override;
  };
}
}
