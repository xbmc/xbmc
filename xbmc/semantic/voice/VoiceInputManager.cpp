/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VoiceInputManager.h"

// Platform-specific implementations
#if defined(TARGET_WINDOWS)
#include "platform/WindowsVoiceInput.h"
#elif defined(TARGET_ANDROID)
#include "platform/AndroidVoiceInput.h"
#elif defined(TARGET_DARWIN)
#include "platform/DarwinVoiceInput.h"
#elif defined(TARGET_LINUX)
#include "platform/LinuxVoiceInput.h"
#else
#include "platform/StubVoiceInput.h"
#endif

#include "utils/log.h"

namespace KODI
{
namespace SEMANTIC
{

std::unique_ptr<IVoiceInput> VoiceInputManager::CreateVoiceInput()
{
#if defined(TARGET_WINDOWS)
  CLog::Log(LOGINFO, "VoiceInputManager: Creating Windows SAPI voice input");
  return std::make_unique<CWindowsVoiceInput>();
#elif defined(TARGET_ANDROID)
  CLog::Log(LOGINFO, "VoiceInputManager: Creating Android voice input");
  return std::make_unique<CAndroidVoiceInput>();
#elif defined(TARGET_DARWIN)
  CLog::Log(LOGINFO, "VoiceInputManager: Creating Darwin voice input");
  return std::make_unique<CDarwinVoiceInput>();
#elif defined(TARGET_LINUX)
  CLog::Log(LOGINFO, "VoiceInputManager: Creating Linux voice input");
  return std::make_unique<CLinuxVoiceInput>();
#else
  CLog::Log(LOGWARNING, "VoiceInputManager: Platform not supported, using stub implementation");
  return std::make_unique<CStubVoiceInput>();
#endif
}

bool VoiceInputManager::IsVoiceInputSupported()
{
#if defined(TARGET_WINDOWS) || defined(TARGET_ANDROID) || defined(TARGET_DARWIN) || \
    defined(TARGET_LINUX)
  return true;
#else
  return false;
#endif
}

std::string VoiceInputManager::GetPlatformName()
{
#if defined(TARGET_WINDOWS)
  return "Windows SAPI";
#elif defined(TARGET_ANDROID)
  return "Android Speech Recognition";
#elif defined(TARGET_DARWIN_IOS)
  return "iOS Speech Framework";
#elif defined(TARGET_DARWIN_TVOS)
  return "tvOS Speech Framework";
#elif defined(TARGET_DARWIN_OSX)
  return "macOS Speech Framework";
#elif defined(TARGET_LINUX)
  return "Linux PocketSphinx";
#else
  return "Not Supported";
#endif
}

std::string VoiceInputManager::ErrorToString(VoiceInputError error)
{
  switch (error)
  {
    case VoiceInputError::None:
      return "No error";
    case VoiceInputError::NoMicrophone:
      return "No microphone detected";
    case VoiceInputError::PermissionDenied:
      return "Microphone permission denied";
    case VoiceInputError::NetworkError:
      return "Network error";
    case VoiceInputError::Timeout:
      return "Recognition timeout";
    case VoiceInputError::NotSupported:
      return "Voice input not supported on this platform";
    case VoiceInputError::InitializationFailed:
      return "Failed to initialize voice input";
    case VoiceInputError::RecognitionFailed:
      return "Voice recognition failed";
    case VoiceInputError::Cancelled:
      return "Voice recognition cancelled";
    default:
      return "Unknown error";
  }
}

std::string VoiceInputManager::StatusToString(VoiceInputStatus status)
{
  switch (status)
  {
    case VoiceInputStatus::Idle:
      return "Idle";
    case VoiceInputStatus::Initializing:
      return "Initializing";
    case VoiceInputStatus::Ready:
      return "Ready";
    case VoiceInputStatus::Listening:
      return "Listening";
    case VoiceInputStatus::Processing:
      return "Processing";
    case VoiceInputStatus::Error:
      return "Error";
    default:
      return "Unknown";
  }
}

} // namespace SEMANTIC
} // namespace KODI
