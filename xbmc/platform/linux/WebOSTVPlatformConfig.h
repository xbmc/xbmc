/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string_view>

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
  static inline std::unique_ptr<HContext, HContextDeleter> m_requestContextSoundOutput{
      new HContext{}};
  static inline std::atomic<bool> m_eAC3Supported{false};
  static inline std::atomic<bool> m_eARCActive{false};
  static inline std::atomic<unsigned int> m_eARCFormats{0};
  static inline std::atomic<std::uint64_t> m_eARCPCM{0};
  static inline std::atomic<bool> m_digitalPassThrough{false};

public:
  static void Load();
  static void LoadARCStatus();
  static void LoadSoundOutputStatus();
  static int GetWebOSVersion();
  static bool SupportsDTS();
  static bool SupportsHDR();
  static bool SupportsEAC3();
  static bool SupportsEARCPassthrough();
  static bool SupportsEARCFormat(std::string_view type);
  static unsigned int GetEARCPCMRate(unsigned int channels, unsigned int sampleRate);
};
