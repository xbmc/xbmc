/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/posix/utils/PosixInterfaceForCLog.h"

class CWasmInterfaceForCLog : public CPosixInterfaceForCLog
{
public:
  CWasmInterfaceForCLog() = default;
  ~CWasmInterfaceForCLog() override = default;

  void AddSinks(
      std::shared_ptr<spdlog::sinks::dist_sink<std::mutex>> distributionSink) const override;
};
