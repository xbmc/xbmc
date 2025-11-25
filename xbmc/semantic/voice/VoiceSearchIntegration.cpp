/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VoiceSearchIntegration.h"

#include "utils/log.h"

namespace KODI
{
namespace SEMANTIC
{

VoiceSearchIntegration::VoiceSearchIntegration() = default;

VoiceSearchIntegration::~VoiceSearchIntegration()
{
  Shutdown();
}

bool VoiceSearchIntegration::Initialize(QueryCallback onQuery, StatusCallback onStatus)
{
  if (!onQuery)
  {
    CLog::Log(LOGERROR, "VoiceSearchIntegration: Query callback is required");
    return false;
  }

  m_onQuery = onQuery;
  m_onStatus = onStatus;

  // Create platform-specific voice input
  m_voiceInput = VoiceInputManager::CreateVoiceInput();
  if (!m_voiceInput)
  {
    CLog::Log(LOGERROR, "VoiceSearchIntegration: Failed to create voice input");
    return false;
  }

  // Set listener
  m_voiceInput->SetListener(this);

  // Initialize
  if (!m_voiceInput->Initialize())
  {
    CLog::Log(LOGWARNING, "VoiceSearchIntegration: Voice input initialization failed");
    return false;
  }

  CLog::Log(LOGINFO, "VoiceSearchIntegration: Initialized with platform: {}",
            m_voiceInput->GetPlatformName());
  return true;
}

void VoiceSearchIntegration::Shutdown()
{
  if (m_voiceInput)
  {
    m_voiceInput->Shutdown();
    m_voiceInput.reset();
  }

  m_onQuery = nullptr;
  m_onStatus = nullptr;
  m_partialText.clear();
  m_status = VoiceInputStatus::Idle;
}

bool VoiceSearchIntegration::IsAvailable() const
{
  return m_voiceInput && m_voiceInput->IsAvailable();
}

bool VoiceSearchIntegration::ToggleListening()
{
  if (!m_voiceInput)
    return false;

  if (m_voiceInput->IsListening())
  {
    StopListening();
    return true;
  }
  else
  {
    return StartListening();
  }
}

bool VoiceSearchIntegration::StartListening()
{
  if (!m_voiceInput || !m_voiceInput->IsAvailable())
  {
    CLog::Log(LOGWARNING, "VoiceSearchIntegration: Voice input not available");
    return false;
  }

  // Check permissions
  if (!m_voiceInput->HasPermissions())
  {
    CLog::Log(LOGWARNING, "VoiceSearchIntegration: Requesting permissions");
    m_voiceInput->RequestPermissions();
    return false;
  }

  m_partialText.clear();
  return m_voiceInput->StartListening();
}

void VoiceSearchIntegration::StopListening()
{
  if (m_voiceInput)
  {
    m_voiceInput->StopListening();
  }
}

bool VoiceSearchIntegration::IsListening() const
{
  return m_voiceInput && m_voiceInput->IsListening();
}

VoiceInputStatus VoiceSearchIntegration::GetStatus() const
{
  return m_status;
}

std::string VoiceSearchIntegration::GetPartialText() const
{
  return m_partialText;
}

bool VoiceSearchIntegration::SetLanguage(const std::string& languageCode)
{
  if (m_voiceInput)
  {
    return m_voiceInput->SetLanguage(languageCode);
  }
  return false;
}

std::vector<VoiceLanguage> VoiceSearchIntegration::GetSupportedLanguages() const
{
  if (m_voiceInput)
  {
    return m_voiceInput->GetSupportedLanguages();
  }
  return {};
}

std::string VoiceSearchIntegration::GetPlatformName() const
{
  if (m_voiceInput)
  {
    return m_voiceInput->GetPlatformName();
  }
  return "Not Available";
}

void VoiceSearchIntegration::OnVoiceStatusChanged(VoiceInputStatus status)
{
  m_status = status;

  if (m_onStatus)
  {
    std::string message = VoiceInputManager::StatusToString(status);
    m_onStatus(status, message);
  }

  CLog::Log(LOGDEBUG, "VoiceSearchIntegration: Status changed to {}",
            VoiceInputManager::StatusToString(status));
}

void VoiceSearchIntegration::OnPartialResult(const VoiceRecognitionResult& result)
{
  m_partialText = result.text;

  CLog::Log(LOGDEBUG, "VoiceSearchIntegration: Partial result: '{}'", result.text);

  if (m_onStatus)
  {
    m_onStatus(VoiceInputStatus::Processing, result.text);
  }
}

void VoiceSearchIntegration::OnFinalResult(const VoiceRecognitionResult& result)
{
  m_partialText.clear();

  CLog::Log(LOGINFO, "VoiceSearchIntegration: Final result: '{}' (confidence: {:.2f})",
            result.text, result.confidence);

  if (m_onQuery)
  {
    m_onQuery(result.text);
  }
}

void VoiceSearchIntegration::OnError(VoiceInputError error, const std::string& message)
{
  m_partialText.clear();

  CLog::Log(LOGERROR, "VoiceSearchIntegration: Error: {}", message);

  if (m_onStatus)
  {
    m_onStatus(VoiceInputStatus::Error, message);
  }
}

void VoiceSearchIntegration::OnVolumeChanged(float level)
{
  // Can be used to display volume level indicator
  CLog::Log(LOGDEBUG, "VoiceSearchIntegration: Volume level: {:.2f}", level);
}

} // namespace SEMANTIC
} // namespace KODI
