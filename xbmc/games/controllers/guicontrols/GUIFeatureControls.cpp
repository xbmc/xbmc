/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFeatureControls.h"

#include "games/controllers/windows/GUIControllerDefines.h"

using namespace KODI;
using namespace GAME;

CGUIFeatureGroupTitle::CGUIFeatureGroupTitle(const CGUILabelControl& groupTitleTemplate,
                                             const std::string& groupName,
                                             unsigned int buttonIndex)
  : CGUILabelControl(groupTitleTemplate)
{
  // Initialize CGUILabelControl
  SetLabel(groupName);
  SetID(CONTROL_FEATURE_GROUPS_START + buttonIndex);
  SetVisible(true);
  AllocResources();
}

CGUIFeatureSeparator::CGUIFeatureSeparator(const CGUIImage& separatorTemplate,
                                           unsigned int buttonIndex)
  : CGUIImage(separatorTemplate)
{
  // Initialize CGUIImage
  SetID(CONTROL_FEATURE_SEPARATORS_START + buttonIndex);
  SetVisible(true);
  AllocResources();
}
