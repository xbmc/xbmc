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
 \brief Basic setting types available in the settings system.
 */
enum class SettingType {
  Unknown = 0,
  Boolean,
  Integer,
  Number,
  String,
  Action,
  List,
  Reference
};
