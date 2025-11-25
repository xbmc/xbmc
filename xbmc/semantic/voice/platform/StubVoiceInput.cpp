/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "StubVoiceInput.h"

#include "utils/log.h"

namespace KODI
{
namespace SEMANTIC
{

bool CStubVoiceInput::Initialize()
{
  CLog::Log(LOGWARNING, "StubVoiceInput: Voice input not supported on this platform");
  m_status = VoiceInputStatus::Error;

  if (m_listener)
  {
    m_listener->OnError(VoiceInputError::NotSupported,
                       "Voice input is not supported on this platform");
  }

  return false;
}

void CStubVoiceInput::Shutdown()
{
  m_status = VoiceInputStatus::Idle;
}

bool CStubVoiceInput::IsAvailable() const
{
  return false;
}

bool CStubVoiceInput::HasPermissions() const
{
  return false;
}

bool CStubVoiceInput::RequestPermissions()
{
  CLog::Log(LOGWARNING, "StubVoiceInput: Cannot request permissions - not supported");
  return false;
}

std::vector<VoiceLanguage> CStubVoiceInput::GetSupportedLanguages() const
{
  return {};
}

bool CStubVoiceInput::SetLanguage(const std::string& languageCode)
{
  m_languageCode = languageCode;
  return false;
}

std::string CStubVoiceInput::GetLanguage() const
{
  return m_languageCode;
}

void CStubVoiceInput::SetMode(VoiceInputMode mode)
{
  m_mode = mode;
}

VoiceInputMode CStubVoiceInput::GetMode() const
{
  return m_mode;
}

bool CStubVoiceInput::StartListening()
{
  CLog::Log(LOGWARNING, "StubVoiceInput: Cannot start listening - not supported");

  if (m_listener)
  {
    m_listener->OnError(VoiceInputError::NotSupported,
                       "Voice input is not supported on this platform");
  }

  return false;
}

void CStubVoiceInput::StopListening()
{
  // No-op
}

void CStubVoiceInput::Cancel()
{
  // No-op
}

bool CStubVoiceInput::IsListening() const
{
  return false;
}

VoiceInputStatus CStubVoiceInput::GetStatus() const
{
  return m_status;
}

void CStubVoiceInput::SetListener(IVoiceInputListener* listener)
{
  m_listener = listener;
}

std::string CStubVoiceInput::GetPlatformName() const
{
  return "Not Supported";
}

} // namespace SEMANTIC
} // namespace KODI
