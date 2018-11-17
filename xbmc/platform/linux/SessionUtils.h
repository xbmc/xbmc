/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

class CSessionUtils
{
public:
  static std::shared_ptr<CSessionUtils> GetSession();

  virtual int Open(const std::string& path, int flags);
  virtual void Close(int fd);

  virtual bool Connect() { return true; }
  virtual void Destroy() {}
};
