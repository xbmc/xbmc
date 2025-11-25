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

#if defined(TARGET_ANDROID)

#include <jni.h>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Android voice input implementation
 *
 * Uses Android SpeechRecognizer API via JNI for voice recognition.
 * Requires RECORD_AUDIO permission.
 */
class CAndroidVoiceInput : public IVoiceInput
{
public:
  CAndroidVoiceInput();
  ~CAndroidVoiceInput() override;

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

  // JNI callbacks (called from Java)
  void OnPartialResults(const std::string& text, float confidence);
  void OnFinalResults(const std::string& text, float confidence);
  void OnError(int errorCode);
  void OnReadyForSpeech();
  void OnBeginningOfSpeech();
  void OnEndOfSpeech();
  void OnRmsChanged(float rmsdB);

private:
  /*!
   * @brief Initialize JNI references
   */
  bool InitializeJNI();

  /*!
   * @brief Check microphone permission
   */
  bool CheckMicrophonePermission();

  // JNI objects
  jobject m_speechRecognizer{nullptr};
  jclass m_voiceInputClass{nullptr};
  jmethodID m_startListeningMethod{nullptr};
  jmethodID m_stopListeningMethod{nullptr};
  jmethodID m_cancelMethod{nullptr};
  jmethodID m_setLanguageMethod{nullptr};

  // State
  IVoiceInputListener* m_listener{nullptr};
  std::atomic<VoiceInputStatus> m_status{VoiceInputStatus::Idle};
  std::atomic<bool> m_isListening{false};
  VoiceInputMode m_mode{VoiceInputMode::PushToTalk};
  std::string m_languageCode{"en-US"};
  bool m_initialized{false};
};

} // namespace SEMANTIC
} // namespace KODI

#endif // TARGET_ANDROID
