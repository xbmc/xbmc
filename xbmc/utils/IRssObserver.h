/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIVecText.h"

class IRssObserver
{
protected:
  /* make sure nobody deletes a pointer to this class */
  ~IRssObserver() = default;

public:
  virtual void OnFeedUpdate(const vecText &feed) = 0;
  virtual void OnFeedRelease() = 0;
};
