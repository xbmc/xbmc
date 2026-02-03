/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/IThreadImpl.h"
#include "threads/SingleLock.h"

#include <string>

class CThreadImplWin : public IThreadImpl
{
public:
  CThreadImplWin(std::thread::native_handle_type handle);

  ~CThreadImplWin() override = default;

  void SetThreadInfo(const std::string& name) override;

  bool SetPriority(const ThreadPriority& priority) override;

  bool SetTask(const ThreadTask& task) override;
  bool RevertTask() override;

private:
  CCriticalSection m_criticalSection;
  std::string m_name;
  HANDLE m_hTask{0};
  DWORD m_taskIndex{0};
};
