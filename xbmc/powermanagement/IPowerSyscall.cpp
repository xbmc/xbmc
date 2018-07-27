/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IPowerSyscall.h"

CreatePowerSyscallFunc IPowerSyscall::m_createFunc = nullptr;

IPowerSyscall* IPowerSyscall::CreateInstance()
{
  if (m_createFunc)
    return m_createFunc();

  return nullptr;
}

void IPowerSyscall::RegisterPowerSyscall(CreatePowerSyscallFunc createFunc)
{
  m_createFunc = createFunc;
}