/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIImage.h"
#include "guilib/GUILabelControl.h"

#include <string>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CGUIFeatureGroupTitle : public CGUILabelControl
{
public:
  CGUIFeatureGroupTitle(const CGUILabelControl& groupTitleTemplate,
                        const std::string& groupName,
                        unsigned int buttonIndex);

  ~CGUIFeatureGroupTitle() override = default;
};

/*!
 * \ingroup games
 */
class CGUIFeatureSeparator : public CGUIImage
{
public:
  CGUIFeatureSeparator(const CGUIImage& separatorTemplate, unsigned int buttonIndex);

  ~CGUIFeatureSeparator() override = default;
};
} // namespace GAME
} // namespace KODI
