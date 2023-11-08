/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIButtonControl.h"

#include <string>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CGUIControllerButton : public CGUIButtonControl
{
public:
  CGUIControllerButton(const CGUIButtonControl& buttonControl,
                       const std::string& label,
                       unsigned int index);

  ~CGUIControllerButton() override = default;
};
} // namespace GAME
} // namespace KODI
