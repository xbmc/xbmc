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

#if defined(TARGET_LINUX) && !defined(TARGET_ANDROID)

// Forward declarations for PocketSphinx
struct ps_decoder_s;
typedef struct ps_decoder_s ps_decoder_t;
struct cmd_ln_s;
typedef struct cmd_ln_s cmd_ln_t;

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Linux voice input implementation
 *
 * Uses PocketSphinx (CMU Sphinx) for offline voice recognition on Linux.
 * Falls back to system speech services if available.
 */
class CLinuxVoiceInput : public IVoiceInput
{
public:
  CLinuxVoiceInput();
  ~CLinuxVoiceInput() override;

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
   * @brief Recognition thread
   */
  void RecognitionThread();

  /*!
   * @brief Process audio data
   */
  void ProcessAudio();

  /*!
   * @brief Check if PocketSphinx is available
   */
  bool CheckPocketSphinx();

  // PocketSphinx objects
  ps_decoder_t* m_decoder{nullptr};
  cmd_ln_t* m_config{nullptr};

  // State
  IVoiceInputListener* m_listener{nullptr};
  std::atomic<VoiceInputStatus> m_status{VoiceInputStatus::Idle};
  std::atomic<bool> m_isListening{false};
  VoiceInputMode m_mode{VoiceInputMode::PushToTalk};
  std::string m_languageCode{"en-US"};

  // Threading
  std::unique_ptr<std::thread> m_thread;
  std::atomic<bool> m_stopThread{false};

  // Audio device
  int m_audioDevice{-1};
  bool m_pocketSphinxAvailable{false};
};

} // namespace SEMANTIC
} // namespace KODI

#endif // TARGET_LINUX && !TARGET_ANDROID
