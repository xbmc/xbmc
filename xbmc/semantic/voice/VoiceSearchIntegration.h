/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IVoiceInput.h"
#include "VoiceInputManager.h"

#include <functional>
#include <memory>
#include <string>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Helper class for integrating voice input into dialogs
 *
 * This class provides a simple interface for adding voice search
 * functionality to any dialog or window. It handles voice input
 * initialization, callbacks, and UI state management.
 *
 * Example usage:
 * \code
 * class MyDialog : public CGUIDialog
 * {
 *   VoiceSearchIntegration m_voiceIntegration;
 *
 *   void OnInitWindow() override
 *   {
 *     m_voiceIntegration.Initialize([this](const std::string& query) {
 *       // Handle voice query
 *       SetSearchQuery(query);
 *     });
 *   }
 *
 *   void OnVoiceButtonClick()
 *   {
 *     m_voiceIntegration.ToggleListening();
 *   }
 * };
 * \endcode
 */
class VoiceSearchIntegration : public IVoiceInputListener
{
public:
  using QueryCallback = std::function<void(const std::string& query)>;
  using StatusCallback = std::function<void(VoiceInputStatus status, const std::string& message)>;

  VoiceSearchIntegration();
  ~VoiceSearchIntegration() override;

  /*!
   * @brief Initialize voice input
   * @param onQuery Callback for final voice query
   * @param onStatus Optional callback for status updates
   * @return true if initialization succeeded
   */
  bool Initialize(QueryCallback onQuery, StatusCallback onStatus = nullptr);

  /*!
   * @brief Shutdown voice input
   */
  void Shutdown();

  /*!
   * @brief Check if voice input is available
   * @return true if available
   */
  bool IsAvailable() const;

  /*!
   * @brief Start/stop voice listening
   * @return true if state changed
   */
  bool ToggleListening();

  /*!
   * @brief Start voice listening
   * @return true if started
   */
  bool StartListening();

  /*!
   * @brief Stop voice listening
   */
  void StopListening();

  /*!
   * @brief Check if currently listening
   * @return true if listening
   */
  bool IsListening() const;

  /*!
   * @brief Get current status
   * @return Current voice input status
   */
  VoiceInputStatus GetStatus() const;

  /*!
   * @brief Get current partial text
   * @return Partial recognition text
   */
  std::string GetPartialText() const;

  /*!
   * @brief Set language
   * @param languageCode Language code (e.g., "en-US")
   * @return true if language was set
   */
  bool SetLanguage(const std::string& languageCode);

  /*!
   * @brief Get supported languages
   * @return List of supported languages
   */
  std::vector<VoiceLanguage> GetSupportedLanguages() const;

  /*!
   * @brief Get platform name
   * @return Platform implementation name
   */
  std::string GetPlatformName() const;

  // IVoiceInputListener implementation
  void OnVoiceStatusChanged(VoiceInputStatus status) override;
  void OnPartialResult(const VoiceRecognitionResult& result) override;
  void OnFinalResult(const VoiceRecognitionResult& result) override;
  void OnError(VoiceInputError error, const std::string& message) override;
  void OnVolumeChanged(float level) override;

private:
  std::unique_ptr<IVoiceInput> m_voiceInput;
  QueryCallback m_onQuery;
  StatusCallback m_onStatus;
  std::string m_partialText;
  VoiceInputStatus m_status{VoiceInputStatus::Idle};
};

} // namespace SEMANTIC
} // namespace KODI
