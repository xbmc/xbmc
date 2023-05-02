/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/IThreadImpl.h"

class CThreadImplLinux : public IThreadImpl
{
public:
  CThreadImplLinux(std::thread::native_handle_type handle);

  ~CThreadImplLinux() override = default;

  void SetThreadInfo(const std::string& name) override;

  bool SetPriority(const ThreadPriority& priority) override;

private:
  pid_t m_threadID;
  std::string m_name;
};
