/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "GUIFeatureControls.h"
#include "games/controllers/windows/GUIControllerDefines.h"

using namespace KODI;
using namespace GAME;

CGUIFeatureGroupTitle::CGUIFeatureGroupTitle(const CGUILabelControl& groupTitleTemplate, const std::string& groupName, unsigned int buttonIndex) :
  CGUILabelControl(groupTitleTemplate)
{
  // Initialize CGUILabelControl
  SetLabel(groupName);
  SetID(CONTROL_FEATURE_GROUPS_START + buttonIndex);
  SetVisible(true);
  AllocResources();
}

CGUIFeatureSeparator::CGUIFeatureSeparator(const CGUIImage& separatorTemplate, unsigned int buttonIndex) :
  CGUIImage(separatorTemplate)
{
  // Initialize CGUIImage
  SetID(CONTROL_FEATURE_SEPARATORS_START + buttonIndex);
  SetVisible(true);
  AllocResources();
}
