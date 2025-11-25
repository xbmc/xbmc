/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WindowsVoiceInput.h"

#if defined(TARGET_WINDOWS)

#include "utils/log.h"

#include <sapi.h>
#include <sphelper.h>
#include <comdef.h>
#include <atlbase.h>

namespace KODI
{
namespace SEMANTIC
{

CWindowsVoiceInput::CWindowsVoiceInput() = default;

CWindowsVoiceInput::~CWindowsVoiceInput()
{
  Shutdown();
}

bool CWindowsVoiceInput::Initialize()
{
  CLog::Log(LOGINFO, "WindowsVoiceInput: Initializing SAPI");

  // Initialize COM
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
  {
    CLog::Log(LOGERROR, "WindowsVoiceInput: Failed to initialize COM: 0x{:08X}", hr);
    return false;
  }
  m_comInitialized = (hr != RPC_E_CHANGED_MODE);

  // Create recognizer
  hr = CoCreateInstance(CLSID_SpInprocRecognizer, nullptr, CLSCTX_ALL,
                       IID_ISpRecognizer, (void**)&m_recognizer);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "WindowsVoiceInput: Failed to create recognizer: 0x{:08X}", hr);
    Shutdown();
    return false;
  }

  // Create recognition context
  hr = m_recognizer->CreateRecoContext(&m_recoContext);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "WindowsVoiceInput: Failed to create context: 0x{:08X}", hr);
    Shutdown();
    return false;
  }

  // Set audio input to default microphone
  CComPtr<ISpObjectToken> audioToken;
  hr = SpGetDefaultTokenFromCategoryId(SPCAT_AUDIOIN, &audioToken);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "WindowsVoiceInput: No microphone detected");
    m_status = VoiceInputStatus::Error;
    if (m_listener)
      m_listener->OnError(VoiceInputError::NoMicrophone, "No microphone detected");
    Shutdown();
    return false;
  }

  hr = m_recognizer->SetInput(audioToken, TRUE);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "WindowsVoiceInput: Failed to set audio input: 0x{:08X}", hr);
    Shutdown();
    return false;
  }

  // Create dictation grammar (free-form speech)
  hr = m_recoContext->CreateGrammar(0, &m_grammar);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "WindowsVoiceInput: Failed to create grammar: 0x{:08X}", hr);
    Shutdown();
    return false;
  }

  hr = m_grammar->LoadDictation(nullptr, SPLO_STATIC);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "WindowsVoiceInput: Failed to load dictation: 0x{:08X}", hr);
    Shutdown();
    return false;
  }

  m_status = VoiceInputStatus::Ready;
  CLog::Log(LOGINFO, "WindowsVoiceInput: Initialized successfully");
  return true;
}

void CWindowsVoiceInput::Shutdown()
{
  CLog::Log(LOGINFO, "WindowsVoiceInput: Shutting down");

  StopListening();

  if (m_grammar)
  {
    m_grammar->Release();
    m_grammar = nullptr;
  }

  if (m_recoContext)
  {
    m_recoContext->Release();
    m_recoContext = nullptr;
  }

  if (m_recognizer)
  {
    m_recognizer->Release();
    m_recognizer = nullptr;
  }

  if (m_comInitialized)
  {
    CoUninitialize();
    m_comInitialized = false;
  }

  m_status = VoiceInputStatus::Idle;
}

bool CWindowsVoiceInput::IsAvailable() const
{
  return m_status == VoiceInputStatus::Ready || m_status == VoiceInputStatus::Listening;
}

bool CWindowsVoiceInput::HasPermissions() const
{
  // Windows doesn't require explicit permission for microphone in desktop apps
  return true;
}

bool CWindowsVoiceInput::RequestPermissions()
{
  // No-op on Windows desktop
  return true;
}

std::vector<VoiceLanguage> CWindowsVoiceInput::GetSupportedLanguages() const
{
  std::vector<VoiceLanguage> languages;

  // Common SAPI languages
  languages.push_back({"en-US", "English (United States)", true});
  languages.push_back({"en-GB", "English (United Kingdom)", false});
  languages.push_back({"fr-FR", "French (France)", false});
  languages.push_back({"de-DE", "German (Germany)", false});
  languages.push_back({"es-ES", "Spanish (Spain)", false});
  languages.push_back({"it-IT", "Italian (Italy)", false});
  languages.push_back({"ja-JP", "Japanese (Japan)", false});
  languages.push_back({"zh-CN", "Chinese (Simplified)", false});

  return languages;
}

bool CWindowsVoiceInput::SetLanguage(const std::string& languageCode)
{
  m_languageCode = languageCode;
  CLog::Log(LOGINFO, "WindowsVoiceInput: Language set to {}", languageCode);
  return true;
}

std::string CWindowsVoiceInput::GetLanguage() const
{
  return m_languageCode;
}

void CWindowsVoiceInput::SetMode(VoiceInputMode mode)
{
  m_mode = mode;
}

VoiceInputMode CWindowsVoiceInput::GetMode() const
{
  return m_mode;
}

bool CWindowsVoiceInput::StartListening()
{
  if (!m_grammar || m_isListening)
    return false;

  CLog::Log(LOGINFO, "WindowsVoiceInput: Starting listening");

  // Activate grammar
  HRESULT hr = m_grammar->SetDictationState(SPRS_ACTIVE);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "WindowsVoiceInput: Failed to activate dictation: 0x{:08X}", hr);
    return false;
  }

  m_isListening = true;
  m_status = VoiceInputStatus::Listening;

  if (m_listener)
    m_listener->OnVoiceStatusChanged(VoiceInputStatus::Listening);

  // Start recognition thread
  m_stopThread = false;
  m_thread = std::make_unique<std::thread>(&CWindowsVoiceInput::RecognitionThread, this);

  return true;
}

void CWindowsVoiceInput::StopListening()
{
  if (!m_isListening)
    return;

  CLog::Log(LOGINFO, "WindowsVoiceInput: Stopping listening");

  m_stopThread = true;
  if (m_thread && m_thread->joinable())
  {
    m_thread->join();
    m_thread.reset();
  }

  if (m_grammar)
  {
    m_grammar->SetDictationState(SPRS_INACTIVE);
  }

  m_isListening = false;
  m_status = VoiceInputStatus::Ready;

  if (m_listener)
    m_listener->OnVoiceStatusChanged(VoiceInputStatus::Ready);
}

void CWindowsVoiceInput::Cancel()
{
  StopListening();
  if (m_listener)
    m_listener->OnError(VoiceInputError::Cancelled, "Recognition cancelled");
}

bool CWindowsVoiceInput::IsListening() const
{
  return m_isListening;
}

VoiceInputStatus CWindowsVoiceInput::GetStatus() const
{
  return m_status;
}

void CWindowsVoiceInput::SetListener(IVoiceInputListener* listener)
{
  m_listener = listener;
}

std::string CWindowsVoiceInput::GetPlatformName() const
{
  return "Windows SAPI 5.4";
}

void CWindowsVoiceInput::RecognitionThread()
{
  CLog::Log(LOGINFO, "WindowsVoiceInput: Recognition thread started");

  while (!m_stopThread && m_recoContext)
  {
    CSpEvent event;

    // Wait for events (100ms timeout)
    HRESULT hr = event.GetFrom(m_recoContext);
    if (hr == S_OK)
    {
      switch (event.eEventId)
      {
        case SPEI_RECOGNITION:
          ProcessResult();
          break;

        case SPEI_HYPOTHESIS:
          // Partial result
          ProcessResult();
          break;

        case SPEI_SOUND_START:
          if (m_listener)
            m_listener->OnVoiceStatusChanged(VoiceInputStatus::Listening);
          break;

        case SPEI_SOUND_END:
          if (m_listener)
            m_listener->OnVoiceStatusChanged(VoiceInputStatus::Processing);
          break;

        default:
          break;
      }
    }

    Sleep(50);
  }

  CLog::Log(LOGINFO, "WindowsVoiceInput: Recognition thread stopped");
}

void CWindowsVoiceInput::ProcessResult()
{
  if (!m_recoContext || !m_listener)
    return;

  CSpEvent event;
  HRESULT hr = event.GetFrom(m_recoContext);
  if (hr != S_OK)
    return;

  ISpRecoResult* result = reinterpret_cast<ISpRecoResult*>(event.lParam);
  if (!result)
    return;

  SPPHRASE* phrase = nullptr;
  hr = result->GetPhrase(&phrase);
  if (SUCCEEDED(hr) && phrase)
  {
    // Get recognized text
    WCHAR* text = nullptr;
    hr = result->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &text, nullptr);
    if (SUCCEEDED(hr) && text)
    {
      // Convert wide string to UTF-8
      int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
      std::string recognizedText(size - 1, 0);
      WideCharToMultiByte(CP_UTF8, 0, text, -1, &recognizedText[0], size, nullptr, nullptr);

      VoiceRecognitionResult voiceResult;
      voiceResult.text = recognizedText;
      voiceResult.confidence = phrase->Rule.SREngineConfidence / 100.0f;
      voiceResult.languageCode = m_languageCode;
      voiceResult.isFinal = (event.eEventId == SPEI_RECOGNITION);

      if (voiceResult.isFinal)
      {
        CLog::Log(LOGINFO, "WindowsVoiceInput: Final result: '{}' (confidence: {:.2f})",
                  recognizedText, voiceResult.confidence);
        m_listener->OnFinalResult(voiceResult);
      }
      else
      {
        m_listener->OnPartialResult(voiceResult);
      }

      CoTaskMemFree(text);
    }

    CoTaskMemFree(phrase);
  }

  result->Release();
}

void CWindowsVoiceInput::UpdateVolume()
{
  // TODO: Implement volume monitoring if needed
}

} // namespace SEMANTIC
} // namespace KODI

#endif // TARGET_WINDOWS
