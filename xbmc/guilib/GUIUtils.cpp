/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIUtils.h"

#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "guilib/GUIComponent.h"
#include "guilib/LocalizeStrings.h"

std::string CGUIUtils::GetLocalizedString(uint32_t id)
{
  if (ADDON::IsSkinStringId(id))
  {
    auto gui = CServiceBroker::GetGUI();
    if (gui)
    {
      auto skin = gui->GetSkinInfo();
      if (skin)
        return g_localizeStrings.GetAddonString(skin->ID(), id);
    }
  }
  return g_localizeStrings.Get(id);
}
