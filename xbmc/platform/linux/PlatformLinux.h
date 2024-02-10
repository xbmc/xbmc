/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/linux/OptionalsReg.h"
#include "platform/posix/PlatformPosix.h"

#include <memory>

class CPlatformLinux : public CPlatformPosix
{
public:
  CPlatformLinux() = default;

  ~CPlatformLinux() override = default;

  bool InitStageOne() override;
  void DeinitStageOne() override;

  bool IsConfigureAddonsAtStartupEnabled() override;

protected:
  virtual void RegisterPowerManagement();

private:
  std::unique_ptr<OPTIONALS::CLircContainer, OPTIONALS::delete_CLircContainer> m_lirc;
};
