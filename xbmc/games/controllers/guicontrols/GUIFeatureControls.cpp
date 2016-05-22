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

#include "GUIFeatureControls.h"
#include "games/controllers/windows/GUIControllerDefines.h"

using namespace GAME;

CGUIFeatureGroupTitle::CGUIFeatureGroupTitle(const CGUILabelControl& groupTitleTemplate, const std::string& groupName, unsigned int featureIndex) :
  CGUILabelControl(groupTitleTemplate)
{
  // Initialize CGUILabelControl
  SetLabel(groupName);
  SetID(CONTROL_FEATURE_GROUPS_START + featureIndex);
  SetVisible(true);
  AllocResources();
}

CGUIFeatureSeparator::CGUIFeatureSeparator(const CGUIImage& separatorTemplate, unsigned int featureIndex) :
  CGUIImage(separatorTemplate)
{
  // Initialize CGUIImage
  SetID(CONTROL_FEATURE_SEPARATORS_START + featureIndex);
  SetVisible(true);
  AllocResources();
}
