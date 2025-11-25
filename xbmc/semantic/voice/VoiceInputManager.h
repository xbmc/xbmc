/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IVoiceInput.h"

#include <memory>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Voice input manager and factory
 *
 * Creates and manages platform-specific voice input implementations.
 * Automatically selects the appropriate implementation based on the
 * current platform.
 *
 * Example usage:
 * \code
 * auto voiceInput = VoiceInputManager::CreateVoiceInput();
 * if (voiceInput && voiceInput->Initialize())
 * {
 *   voiceInput->SetListener(myListener);
 *   voiceInput->StartListening();
 * }
 * \endcode
 */
class VoiceInputManager
{
public:
  /*!
   * @brief Create a platform-specific voice input instance
   * @return Unique pointer to voice input implementation
   */
  static std::unique_ptr<IVoiceInput> CreateVoiceInput();

  /*!
   * @brief Check if voice input is supported on the current platform
   * @return true if voice input is supported
   */
  static bool IsVoiceInputSupported();

  /*!
   * @brief Get the platform name for the current voice input implementation
   * @return Platform name
   */
  static std::string GetPlatformName();

  /*!
   * @brief Convert error code to human-readable string
   * @param error Error code
   * @return Error message string
   */
  static std::string ErrorToString(VoiceInputError error);

  /*!
   * @brief Convert status to human-readable string
   * @param status Status code
   * @return Status string
   */
  static std::string StatusToString(VoiceInputStatus status);
};

} // namespace SEMANTIC
} // namespace KODI
