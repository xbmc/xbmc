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
#pragma once

#include "guilib/GUILabelControl.h"
#include "guilib/GUIImage.h"

#include <string>

namespace GAME
{
  class CGUIFeatureGroupTitle : public CGUILabelControl
  {
  public:
    CGUIFeatureGroupTitle(const CGUILabelControl& groupTitleTemplate, const std::string& groupName, unsigned int featureIndex);

    virtual ~CGUIFeatureGroupTitle(void) { }
  };

  class CGUIFeatureSeparator : public CGUIImage
  {
  public:
    CGUIFeatureSeparator(const CGUIImage& separatorTemplate, unsigned int featureIndex);

    virtual ~CGUIFeatureSeparator(void) { }
  };
}
