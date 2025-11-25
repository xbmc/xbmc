/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LinuxVoiceInput.h"

#if defined(TARGET_LINUX) && !defined(TARGET_ANDROID)

#include "utils/log.h"

#ifdef HAS_POCKETSPHINX
#include <pocketsphinx.h>
#include <sphinxbase/ad.h>
#endif

#include <chrono>
#include <thread>

namespace KODI
{
namespace SEMANTIC
{

CLinuxVoiceInput::CLinuxVoiceInput() = default;

CLinuxVoiceInput::~CLinuxVoiceInput()
{
  Shutdown();
}

bool CLinuxVoiceInput::Initialize()
{
  CLog::Log(LOGINFO, "LinuxVoiceInput: Initializing");

  m_pocketSphinxAvailable = CheckPocketSphinx();

  if (!m_pocketSphinxAvailable)
  {
    CLog::Log(LOGWARNING, "LinuxVoiceInput: PocketSphinx not available");
    m_status = VoiceInputStatus::Error;
    if (m_listener)
      m_listener->OnError(VoiceInputError::NotSupported,
                         "PocketSphinx not available. Install pocketsphinx package.");
    return false;
  }

#ifdef HAS_POCKETSPHINX
  // Create configuration
  m_config = cmd_ln_init(nullptr, ps_args(), TRUE,
                        "-hmm", MODELDIR "/en-us/en-us",
                        "-lm", MODELDIR "/en-us/en-us.lm.bin",
                        "-dict", MODELDIR "/en-us/cmudict-en-us.dict",
                        nullptr);

  if (!m_config)
  {
    CLog::Log(LOGERROR, "LinuxVoiceInput: Failed to create configuration");
    m_status = VoiceInputStatus::Error;
    if (m_listener)
      m_listener->OnError(VoiceInputError::InitializationFailed,
                         "Failed to initialize speech models");
    return false;
  }

  // Create decoder
  m_decoder = ps_init(m_config);
  if (!m_decoder)
  {
    CLog::Log(LOGERROR, "LinuxVoiceInput: Failed to create decoder");
    cmd_ln_free_r(m_config);
    m_config = nullptr;
    m_status = VoiceInputStatus::Error;
    if (m_listener)
      m_listener->OnError(VoiceInputError::InitializationFailed,
                         "Failed to initialize speech decoder");
    return false;
  }

  m_status = VoiceInputStatus::Ready;
  CLog::Log(LOGINFO, "LinuxVoiceInput: Initialized successfully");
  return true;
#else
  CLog::Log(LOGERROR, "LinuxVoiceInput: Compiled without PocketSphinx support");
  m_status = VoiceInputStatus::Error;
  if (m_listener)
    m_listener->OnError(VoiceInputError::NotSupported,
                       "Voice input not supported on this build");
  return false;
#endif
}

void CLinuxVoiceInput::Shutdown()
{
  CLog::Log(LOGINFO, "LinuxVoiceInput: Shutting down");

  StopListening();

#ifdef HAS_POCKETSPHINX
  if (m_decoder)
  {
    ps_free(m_decoder);
    m_decoder = nullptr;
  }

  if (m_config)
  {
    cmd_ln_free_r(m_config);
    m_config = nullptr;
  }
#endif

  m_status = VoiceInputStatus::Idle;
}

bool CLinuxVoiceInput::IsAvailable() const
{
  return m_pocketSphinxAvailable &&
         (m_status == VoiceInputStatus::Ready || m_status == VoiceInputStatus::Listening);
}

bool CLinuxVoiceInput::HasPermissions() const
{
  // Linux doesn't require explicit runtime permissions for microphone
  return true;
}

bool CLinuxVoiceInput::RequestPermissions()
{
  // No-op on Linux
  return true;
}

std::vector<VoiceLanguage> CLinuxVoiceInput::GetSupportedLanguages() const
{
  std::vector<VoiceLanguage> languages;

  // PocketSphinx supports multiple languages if models are installed
  languages.push_back({"en-US", "English (United States)", true});
  // Additional languages would require their model files
  // languages.push_back({"es-ES", "Spanish (Spain)", false});
  // languages.push_back({"fr-FR", "French (France)", false});
  // languages.push_back({"de-DE", "German (Germany)", false});

  return languages;
}

bool CLinuxVoiceInput::SetLanguage(const std::string& languageCode)
{
  m_languageCode = languageCode;
  CLog::Log(LOGINFO, "LinuxVoiceInput: Language set to {}", languageCode);
  // Note: Changing language would require reloading models
  return true;
}

std::string CLinuxVoiceInput::GetLanguage() const
{
  return m_languageCode;
}

void CLinuxVoiceInput::SetMode(VoiceInputMode mode)
{
  m_mode = mode;
}

VoiceInputMode CLinuxVoiceInput::GetMode() const
{
  return m_mode;
}

bool CLinuxVoiceInput::StartListening()
{
#ifdef HAS_POCKETSPHINX
  if (!m_decoder || m_isListening)
    return false;

  CLog::Log(LOGINFO, "LinuxVoiceInput: Starting listening");

  // Start utterance
  if (ps_start_utt(m_decoder) < 0)
  {
    CLog::Log(LOGERROR, "LinuxVoiceInput: Failed to start utterance");
    return false;
  }

  m_isListening = true;
  m_status = VoiceInputStatus::Listening;

  if (m_listener)
    m_listener->OnVoiceStatusChanged(VoiceInputStatus::Listening);

  // Start recognition thread
  m_stopThread = false;
  m_thread = std::make_unique<std::thread>(&CLinuxVoiceInput::RecognitionThread, this);

  return true;
#else
  return false;
#endif
}

void CLinuxVoiceInput::StopListening()
{
  if (!m_isListening)
    return;

  CLog::Log(LOGINFO, "LinuxVoiceInput: Stopping listening");

  m_stopThread = true;
  if (m_thread && m_thread->joinable())
  {
    m_thread->join();
    m_thread.reset();
  }

#ifdef HAS_POCKETSPHINX
  if (m_decoder)
  {
    ps_end_utt(m_decoder);
  }
#endif

  m_isListening = false;
  m_status = VoiceInputStatus::Ready;

  if (m_listener)
    m_listener->OnVoiceStatusChanged(VoiceInputStatus::Ready);
}

void CLinuxVoiceInput::Cancel()
{
  StopListening();
  if (m_listener)
    m_listener->OnError(VoiceInputError::Cancelled, "Recognition cancelled");
}

bool CLinuxVoiceInput::IsListening() const
{
  return m_isListening;
}

VoiceInputStatus CLinuxVoiceInput::GetStatus() const
{
  return m_status;
}

void CLinuxVoiceInput::SetListener(IVoiceInputListener* listener)
{
  m_listener = listener;
}

std::string CLinuxVoiceInput::GetPlatformName() const
{
  return "Linux PocketSphinx";
}

bool CLinuxVoiceInput::CheckPocketSphinx()
{
#ifdef HAS_POCKETSPHINX
  // Check if we can create a basic configuration
  cmd_ln_t* test_config = cmd_ln_init(nullptr, ps_args(), TRUE, nullptr);
  if (test_config)
  {
    cmd_ln_free_r(test_config);
    return true;
  }
#endif
  return false;
}

void CLinuxVoiceInput::RecognitionThread()
{
  CLog::Log(LOGINFO, "LinuxVoiceInput: Recognition thread started");

#ifdef HAS_POCKETSPHINX
  // Open audio device
  ad_rec_t* ad = ad_open_dev(cmd_ln_str_r(m_config, "-adcdev"),
                             (int)cmd_ln_float32_r(m_config, "-samprate"));
  if (!ad)
  {
    CLog::Log(LOGERROR, "LinuxVoiceInput: Failed to open audio device");
    if (m_listener)
      m_listener->OnError(VoiceInputError::NoMicrophone, "Failed to open microphone");
    return;
  }

  // Start recording
  if (ad_start_rec(ad) < 0)
  {
    CLog::Log(LOGERROR, "LinuxVoiceInput: Failed to start recording");
    ad_close(ad);
    return;
  }

  const int16_t* buf;
  int32_t nsamples;

  while (!m_stopThread)
  {
    // Read audio samples
    if ((nsamples = ad_read(ad, (int16_t**)&buf, 2048)) >= 0)
    {
      ps_process_raw(m_decoder, buf, nsamples, FALSE, FALSE);

      // Check for hypothesis
      int32_t score;
      const char* hyp = ps_get_hyp(m_decoder, &score);

      if (hyp != nullptr)
      {
        std::string text(hyp);
        if (!text.empty() && m_listener)
        {
          VoiceRecognitionResult result;
          result.text = text;
          result.confidence = static_cast<float>(score) / -100000.0f; // Normalize score
          result.languageCode = m_languageCode;
          result.isFinal = false;

          m_listener->OnPartialResult(result);
        }
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // Stop recording
  ad_stop_rec(ad);
  ad_close(ad);

  // Get final result
  int32_t score;
  const char* hyp = ps_get_hyp(m_decoder, &score);
  if (hyp != nullptr && m_listener)
  {
    std::string text(hyp);
    if (!text.empty())
    {
      VoiceRecognitionResult result;
      result.text = text;
      result.confidence = static_cast<float>(score) / -100000.0f;
      result.languageCode = m_languageCode;
      result.isFinal = true;

      CLog::Log(LOGINFO, "LinuxVoiceInput: Final result: '{}' (score: {})", text, score);
      m_listener->OnFinalResult(result);
    }
  }
#endif

  CLog::Log(LOGINFO, "LinuxVoiceInput: Recognition thread stopped");
}

void CLinuxVoiceInput::ProcessAudio()
{
  // Implemented in RecognitionThread
}

} // namespace SEMANTIC
} // namespace KODI

#endif // TARGET_LINUX && !TARGET_ANDROID
