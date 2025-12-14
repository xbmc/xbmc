/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <atomic>
#include <memory>

#include <webos-helpers/libhelpers.h>

class CVariant;

struct HContextDeleter
{
  void operator()(HContext* ctx) const noexcept { HUnregisterServiceCallback(ctx); }
};

class WebOSTVPlatformConfig
{
private:
  static inline std::unique_ptr<HContext, HContextDeleter> m_requestContextARC{new HContext{}};
  static inline std::atomic<bool> m_eAC3Supported{false};

public:
  static void Load();
  static void LoadARCStatus();
  static int GetWebOSVersion();
  static bool SupportsDTS();
  static bool SupportsHDR();
  static bool SupportsEAC3();
};
