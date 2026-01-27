/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIHelpers.h"

#include "ServiceBroker.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonInfo.h"
#include "dialogs/GUIDialogYesNo.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"

using namespace ADDON;
using namespace ADDON::GUI;

bool CHelpers::DialogAddonLifecycleUseAsk(const std::shared_ptr<const IAddon>& addon)
{
  int header_nr;
  int text_nr;
  switch (addon->LifecycleState())
  {
    case AddonLifecycleState::BROKEN:
      header_nr = 24164;
      text_nr = 24165;
      break;
    case AddonLifecycleState::DEPRECATED:
      header_nr = 24166;
      text_nr = 24167;
      break;
    default:
      header_nr = 0;
      text_nr = 0;
      break;
  }
  if (header_nr > 0)
  {
    std::string header = StringUtils::Format(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(header_nr), addon->ID());
    std::string text = StringUtils::Format(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(text_nr),
        addon->LifecycleStateDescription());
    if (!CGUIDialogYesNo::ShowAndGetInput(header, text))
      return false;
  }

  return true;
}
