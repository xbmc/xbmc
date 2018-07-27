/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUILabelControl.h"
#include "guilib/GUIImage.h"

#include <string>

namespace KODI
{
namespace GAME
{
  class CGUIFeatureGroupTitle : public CGUILabelControl
  {
  public:
    CGUIFeatureGroupTitle(const CGUILabelControl& groupTitleTemplate, const std::string& groupName, unsigned int buttonIndex);

    virtual ~CGUIFeatureGroupTitle() = default;
  };

  class CGUIFeatureSeparator : public CGUIImage
  {
  public:
    CGUIFeatureSeparator(const CGUIImage& separatorTemplate, unsigned int buttonIndex);

    virtual ~CGUIFeatureSeparator() = default;
  };
}
}
