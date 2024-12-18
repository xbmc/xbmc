/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "cores/RetroPlayer/process/RPProcessInfo.h"

namespace KODI
{
namespace RETRO
{
class CRPProcessInfoEGL : public CRPProcessInfo
{
public:
  CRPProcessInfoEGL(std::string platformName);
  ~CRPProcessInfoEGL() override = default;

  // Implementation of CRPProcessInfo
  HwProcedureAddress GetHwProcedureAddress(const char* symbol) override;
};
} // namespace RETRO
} // namespace KODI
