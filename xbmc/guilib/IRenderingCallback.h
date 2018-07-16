/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class IRenderingCallback
{
public:
  virtual ~IRenderingCallback() = default;
  virtual bool Create(int x, int y, int w, int h, void *device) = 0;
  virtual void Render() = 0;
  virtual void Stop() = 0;
  virtual bool IsDirty() { return true; }
};
