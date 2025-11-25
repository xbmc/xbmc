/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "semantic/voice/IVoiceInput.h"

#include <atomic>
#include <string>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Stub voice input implementation
 *
 * Fallback implementation for platforms that don't support voice input.
 * Always returns errors indicating voice input is not supported.
 */
class CStubVoiceInput : public IVoiceInput
{
public:
  CStubVoiceInput() = default;
  ~CStubVoiceInput() override = default;

  // IVoiceInput implementation
  bool Initialize() override;
  void Shutdown() override;
  bool IsAvailable() const override;
  bool HasPermissions() const override;
  bool RequestPermissions() override;
  std::vector<VoiceLanguage> GetSupportedLanguages() const override;
  bool SetLanguage(const std::string& languageCode) override;
  std::string GetLanguage() const override;
  void SetMode(VoiceInputMode mode) override;
  VoiceInputMode GetMode() const override;
  bool StartListening() override;
  void StopListening() override;
  void Cancel() override;
  bool IsListening() const override;
  VoiceInputStatus GetStatus() const override;
  void SetListener(IVoiceInputListener* listener) override;
  std::string GetPlatformName() const override;

private:
  IVoiceInputListener* m_listener{nullptr};
  std::atomic<VoiceInputStatus> m_status{VoiceInputStatus::Idle};
  VoiceInputMode m_mode{VoiceInputMode::PushToTalk};
  std::string m_languageCode{"en-US"};
};

} // namespace SEMANTIC
} // namespace KODI
