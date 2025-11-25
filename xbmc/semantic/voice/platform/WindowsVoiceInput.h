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
#include <thread>

#if defined(TARGET_WINDOWS)

// Forward declarations for COM interfaces
struct ISpRecognizer;
struct ISpRecoContext;
struct ISpRecoGrammar;

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Windows SAPI voice input implementation
 *
 * Uses Microsoft Speech API (SAPI) for voice recognition on Windows.
 * Supports Windows Vista and later with SAPI 5.4+.
 */
class CWindowsVoiceInput : public IVoiceInput
{
public:
  CWindowsVoiceInput();
  ~CWindowsVoiceInput() override;

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
  /*!
   * @brief Recognition event loop
   */
  void RecognitionThread();

  /*!
   * @brief Process a recognition result
   */
  void ProcessResult();

  /*!
   * @brief Update volume level
   */
  void UpdateVolume();

  // SAPI COM interfaces
  ISpRecognizer* m_recognizer{nullptr};
  ISpRecoContext* m_recoContext{nullptr};
  ISpRecoGrammar* m_grammar{nullptr};

  // State
  IVoiceInputListener* m_listener{nullptr};
  std::atomic<VoiceInputStatus> m_status{VoiceInputStatus::Idle};
  std::atomic<bool> m_isListening{false};
  VoiceInputMode m_mode{VoiceInputMode::PushToTalk};
  std::string m_languageCode{"en-US"};

  // Threading
  std::unique_ptr<std::thread> m_thread;
  std::atomic<bool> m_stopThread{false};

  // COM initialization
  bool m_comInitialized{false};
};

} // namespace SEMANTIC
} // namespace KODI

#endif // TARGET_WINDOWS
