/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
 \ingroup settings
 \brief Levels which every setting is assigned to.
 */
enum class SettingLevel {
  Basic = 0,
  Standard,
  Advanced,
  Expert,
  Internal
};
