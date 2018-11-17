/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/linux/DBusUtil.h"
#include "platform/linux/SessionUtils.h"

class CLogindUtils : public CSessionUtils
{
public:
  CLogindUtils() = default;
  ~CLogindUtils() = default;

  int Open(const std::string& path, int flags) override;
  void Close(int fd) override;

  bool Connect() override;
  void Destroy() override;

private:
  std::string GetSessionPath();

  int TakeDevice(const std::string& sessionPath, uint32_t major, uint32_t minor);
  void ReleaseDevice(const std::string& sessionPath, uint32_t major, uint32_t minor);

  bool TakeControl();
  void ReleaseControl();

  bool Activate();

  std::string m_sessionPath;

  CDBusConnection m_connection;
};
