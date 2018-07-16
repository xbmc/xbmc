/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

class ISettingControl;

/*!
 \ingroup settings
 \brief Interface for creating a new setting control of a custom setting control type.
 */
class ISettingControlCreator
{
public:
  virtual ~ISettingControlCreator() = default;

  /*!
   \brief Creates a new setting control of the given custom setting control type.

   \param controlType string representation of the setting control type
   \return A new setting control object of the given (custom) setting control type or nullptr if the setting control type is unknown
   */
  virtual std::shared_ptr<ISettingControl> CreateControl(const std::string &controlType) const = 0;
};
