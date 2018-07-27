/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CAction;

class IActionListener
{
public:
  virtual ~IActionListener() = default;

  virtual bool OnAction(const CAction &action) = 0;
};
