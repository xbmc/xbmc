/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <chrono>
#include <memory>
#include <string>

namespace KODI
{
namespace KEYMAP
{
class CIRTranslator;
} // namespace KEYMAP
} // namespace KODI

class CLirc : CThread
{
public:
  CLirc();
  ~CLirc() override;
  void Start();

protected:
  void Process() override;
  void ProcessCode(char *buf);
  bool CheckDaemon();

  int m_fd = -1;
  std::chrono::time_point<std::chrono::steady_clock> m_firstClickTime;
  CCriticalSection m_critSection;
  std::unique_ptr<KODI::KEYMAP::CIRTranslator> m_irTranslator;
  int m_profileId;
};
