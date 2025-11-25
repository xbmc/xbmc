/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Voice recognition error codes
 */
enum class VoiceInputError
{
  None = 0,
  NoMicrophone,
  PermissionDenied,
  NetworkError,
  Timeout,
  NotSupported,
  InitializationFailed,
  RecognitionFailed,
  Cancelled
};

/*!
 * @brief Voice recognition status
 */
enum class VoiceInputStatus
{
  Idle,
  Initializing,
  Ready,
  Listening,
  Processing,
  Error
};

/*!
 * @brief Voice input mode
 */
enum class VoiceInputMode
{
  PushToTalk,   // User manually starts/stops listening
  HandsFree     // Automatic voice activation
};

/*!
 * @brief Supported languages for voice recognition
 */
struct VoiceLanguage
{
  std::string code;        // Language code (e.g., "en-US", "fr-FR")
  std::string displayName; // Display name (e.g., "English (US)")
  bool isDefault{false};   // Is this the default language
};

/*!
 * @brief Voice recognition result
 */
struct VoiceRecognitionResult
{
  std::string text;              // Recognized text
  float confidence{0.0f};        // Confidence score (0.0-1.0)
  std::string languageCode;      // Detected language
  bool isFinal{true};            // Is this a final result or partial?
  std::vector<std::string> alternatives; // Alternative interpretations
};

/*!
 * @brief Callback interface for voice input events
 */
class IVoiceInputListener
{
public:
  virtual ~IVoiceInputListener() = default;

  /*!
   * @brief Called when voice recognition status changes
   * @param status New status
   */
  virtual void OnVoiceStatusChanged(VoiceInputStatus status) = 0;

  /*!
   * @brief Called when partial recognition results are available
   * @param result Partial recognition result
   */
  virtual void OnPartialResult(const VoiceRecognitionResult& result) = 0;

  /*!
   * @brief Called when final recognition results are available
   * @param result Final recognition result
   */
  virtual void OnFinalResult(const VoiceRecognitionResult& result) = 0;

  /*!
   * @brief Called when an error occurs
   * @param error Error code
   * @param message Error message
   */
  virtual void OnError(VoiceInputError error, const std::string& message) = 0;

  /*!
   * @brief Called when the volume level changes during listening
   * @param level Volume level (0.0-1.0)
   */
  virtual void OnVolumeChanged(float level) = 0;
};

/*!
 * @brief Voice input interface
 *
 * Platform-agnostic interface for voice recognition functionality.
 * Each platform implements this interface to provide voice input capabilities.
 */
class IVoiceInput
{
public:
  virtual ~IVoiceInput() = default;

  /*!
   * @brief Initialize the voice input system
   * @return true if initialization succeeded
   */
  virtual bool Initialize() = 0;

  /*!
   * @brief Shutdown the voice input system
   */
  virtual void Shutdown() = 0;

  /*!
   * @brief Check if voice input is available on this platform
   * @return true if voice input is supported and available
   */
  virtual bool IsAvailable() const = 0;

  /*!
   * @brief Check if voice input has necessary permissions
   * @return true if permissions are granted
   */
  virtual bool HasPermissions() const = 0;

  /*!
   * @brief Request permissions for voice input (async)
   * @return true if permission request was initiated
   */
  virtual bool RequestPermissions() = 0;

  /*!
   * @brief Get supported languages
   * @return List of supported languages
   */
  virtual std::vector<VoiceLanguage> GetSupportedLanguages() const = 0;

  /*!
   * @brief Set the recognition language
   * @param languageCode Language code (e.g., "en-US")
   * @return true if language was set successfully
   */
  virtual bool SetLanguage(const std::string& languageCode) = 0;

  /*!
   * @brief Get the current language
   * @return Current language code
   */
  virtual std::string GetLanguage() const = 0;

  /*!
   * @brief Set the input mode
   * @param mode Voice input mode
   */
  virtual void SetMode(VoiceInputMode mode) = 0;

  /*!
   * @brief Get the current input mode
   * @return Current input mode
   */
  virtual VoiceInputMode GetMode() const = 0;

  /*!
   * @brief Start listening for voice input
   * @return true if listening started successfully
   */
  virtual bool StartListening() = 0;

  /*!
   * @brief Stop listening for voice input
   */
  virtual void StopListening() = 0;

  /*!
   * @brief Cancel current voice recognition
   */
  virtual void Cancel() = 0;

  /*!
   * @brief Check if currently listening
   * @return true if actively listening
   */
  virtual bool IsListening() const = 0;

  /*!
   * @brief Get current status
   * @return Current voice input status
   */
  virtual VoiceInputStatus GetStatus() const = 0;

  /*!
   * @brief Set the event listener
   * @param listener Listener for voice input events
   */
  virtual void SetListener(IVoiceInputListener* listener) = 0;

  /*!
   * @brief Get the platform name for this implementation
   * @return Platform name (e.g., "Windows SAPI", "Android Speech")
   */
  virtual std::string GetPlatformName() const = 0;
};

} // namespace SEMANTIC
} // namespace KODI
