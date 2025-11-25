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
#include <memory>
#include <string>

#if defined(TARGET_DARWIN)

// Forward declaration for Objective-C objects
#ifdef __OBJC__
@class DarwinVoiceInputImpl;
#else
typedef void DarwinVoiceInputImpl;
#endif

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Darwin (macOS/iOS) voice input implementation
 *
 * Uses Apple Speech framework (NSSpeechRecognizer on macOS,
 * SFSpeechRecognizer on iOS) for voice recognition.
 * Requires microphone permission on iOS.
 */
class CDarwinVoiceInput : public IVoiceInput
{
public:
  CDarwinVoiceInput();
  ~CDarwinVoiceInput() override;

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

  // Callbacks from Objective-C implementation
  void OnPartialResults(const std::string& text, float confidence);
  void OnFinalResults(const std::string& text, float confidence);
  void OnError(int errorCode, const std::string& message);
  void OnStatusChanged(VoiceInputStatus status);
  void OnVolumeChanged(float level);

private:
  // Objective-C implementation (opaque pointer)
  DarwinVoiceInputImpl* m_impl{nullptr};

  // State
  IVoiceInputListener* m_listener{nullptr};
  std::atomic<VoiceInputStatus> m_status{VoiceInputStatus::Idle};
  std::atomic<bool> m_isListening{false};
  VoiceInputMode m_mode{VoiceInputMode::PushToTalk};
  std::string m_languageCode{"en-US"};
};

} // namespace SEMANTIC
} // namespace KODI

#endif // TARGET_DARWIN
