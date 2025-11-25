/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidVoiceInput.h"

#if defined(TARGET_ANDROID)

#include "platform/android/activity/XBMCApp.h"
#include "utils/log.h"

namespace KODI
{
namespace SEMANTIC
{

CAndroidVoiceInput::CAndroidVoiceInput() = default;

CAndroidVoiceInput::~CAndroidVoiceInput()
{
  Shutdown();
}

bool CAndroidVoiceInput::Initialize()
{
  CLog::Log(LOGINFO, "AndroidVoiceInput: Initializing");

  if (!InitializeJNI())
  {
    CLog::Log(LOGERROR, "AndroidVoiceInput: Failed to initialize JNI");
    return false;
  }

  if (!CheckMicrophonePermission())
  {
    CLog::Log(LOGWARNING, "AndroidVoiceInput: Microphone permission not granted");
    m_status = VoiceInputStatus::Error;
    if (m_listener)
      m_listener->OnError(VoiceInputError::PermissionDenied, "Microphone permission required");
    return false;
  }

  m_initialized = true;
  m_status = VoiceInputStatus::Ready;
  CLog::Log(LOGINFO, "AndroidVoiceInput: Initialized successfully");
  return true;
}

void CAndroidVoiceInput::Shutdown()
{
  CLog::Log(LOGINFO, "AndroidVoiceInput: Shutting down");

  StopListening();

  JNIEnv* env = xbmc_jnienv();
  if (env)
  {
    if (m_speechRecognizer)
    {
      env->DeleteGlobalRef(m_speechRecognizer);
      m_speechRecognizer = nullptr;
    }
    if (m_voiceInputClass)
    {
      env->DeleteGlobalRef(m_voiceInputClass);
      m_voiceInputClass = nullptr;
    }
  }

  m_initialized = false;
  m_status = VoiceInputStatus::Idle;
}

bool CAndroidVoiceInput::IsAvailable() const
{
  return m_initialized && (m_status == VoiceInputStatus::Ready ||
                          m_status == VoiceInputStatus::Listening);
}

bool CAndroidVoiceInput::HasPermissions() const
{
  return CheckMicrophonePermission();
}

bool CAndroidVoiceInput::RequestPermissions()
{
  CLog::Log(LOGINFO, "AndroidVoiceInput: Requesting microphone permission");

  // Request permission via Android activity
  JNIEnv* env = xbmc_jnienv();
  if (!env)
    return false;

  jclass activityClass = env->FindClass("org/xbmc/kodi/MainActivity");
  if (!activityClass)
    return false;

  jmethodID requestMethod = env->GetStaticMethodID(
      activityClass, "requestMicrophonePermission", "()V");
  if (requestMethod)
  {
    env->CallStaticVoidMethod(activityClass, requestMethod);
    env->DeleteLocalRef(activityClass);
    return true;
  }

  env->DeleteLocalRef(activityClass);
  return false;
}

std::vector<VoiceLanguage> CAndroidVoiceInput::GetSupportedLanguages() const
{
  std::vector<VoiceLanguage> languages;

  // Common Android speech recognition languages
  languages.push_back({"en-US", "English (United States)", true});
  languages.push_back({"en-GB", "English (United Kingdom)", false});
  languages.push_back({"fr-FR", "French (France)", false});
  languages.push_back({"de-DE", "German (Germany)", false});
  languages.push_back({"es-ES", "Spanish (Spain)", false});
  languages.push_back({"it-IT", "Italian (Italy)", false});
  languages.push_back({"ja-JP", "Japanese (Japan)", false});
  languages.push_back({"zh-CN", "Chinese (Simplified)", false});
  languages.push_back({"ko-KR", "Korean (Korea)", false});
  languages.push_back({"pt-BR", "Portuguese (Brazil)", false});

  return languages;
}

bool CAndroidVoiceInput::SetLanguage(const std::string& languageCode)
{
  m_languageCode = languageCode;

  if (m_speechRecognizer && m_setLanguageMethod)
  {
    JNIEnv* env = xbmc_jnienv();
    if (env)
    {
      jstring jLangCode = env->NewStringUTF(languageCode.c_str());
      env->CallVoidMethod(m_speechRecognizer, m_setLanguageMethod, jLangCode);
      env->DeleteLocalRef(jLangCode);
    }
  }

  CLog::Log(LOGINFO, "AndroidVoiceInput: Language set to {}", languageCode);
  return true;
}

std::string CAndroidVoiceInput::GetLanguage() const
{
  return m_languageCode;
}

void CAndroidVoiceInput::SetMode(VoiceInputMode mode)
{
  m_mode = mode;
}

VoiceInputMode CAndroidVoiceInput::GetMode() const
{
  return m_mode;
}

bool CAndroidVoiceInput::StartListening()
{
  if (!m_initialized || !m_speechRecognizer || m_isListening)
    return false;

  CLog::Log(LOGINFO, "AndroidVoiceInput: Starting listening");

  JNIEnv* env = xbmc_jnienv();
  if (!env || !m_startListeningMethod)
    return false;

  env->CallVoidMethod(m_speechRecognizer, m_startListeningMethod);

  m_isListening = true;
  m_status = VoiceInputStatus::Listening;

  if (m_listener)
    m_listener->OnVoiceStatusChanged(VoiceInputStatus::Listening);

  return true;
}

void CAndroidVoiceInput::StopListening()
{
  if (!m_isListening)
    return;

  CLog::Log(LOGINFO, "AndroidVoiceInput: Stopping listening");

  JNIEnv* env = xbmc_jnienv();
  if (env && m_speechRecognizer && m_stopListeningMethod)
  {
    env->CallVoidMethod(m_speechRecognizer, m_stopListeningMethod);
  }

  m_isListening = false;
  m_status = VoiceInputStatus::Ready;

  if (m_listener)
    m_listener->OnVoiceStatusChanged(VoiceInputStatus::Ready);
}

void CAndroidVoiceInput::Cancel()
{
  CLog::Log(LOGINFO, "AndroidVoiceInput: Cancelling");

  JNIEnv* env = xbmc_jnienv();
  if (env && m_speechRecognizer && m_cancelMethod)
  {
    env->CallVoidMethod(m_speechRecognizer, m_cancelMethod);
  }

  m_isListening = false;
  m_status = VoiceInputStatus::Ready;

  if (m_listener)
  {
    m_listener->OnError(VoiceInputError::Cancelled, "Recognition cancelled");
    m_listener->OnVoiceStatusChanged(VoiceInputStatus::Ready);
  }
}

bool CAndroidVoiceInput::IsListening() const
{
  return m_isListening;
}

VoiceInputStatus CAndroidVoiceInput::GetStatus() const
{
  return m_status;
}

void CAndroidVoiceInput::SetListener(IVoiceInputListener* listener)
{
  m_listener = listener;
}

std::string CAndroidVoiceInput::GetPlatformName() const
{
  return "Android SpeechRecognizer";
}

bool CAndroidVoiceInput::InitializeJNI()
{
  JNIEnv* env = xbmc_jnienv();
  if (!env)
  {
    CLog::Log(LOGERROR, "AndroidVoiceInput: Failed to get JNI environment");
    return false;
  }

  // Find the voice input helper class (would need to be implemented in Java)
  jclass localClass = env->FindClass("org/xbmc/kodi/voice/VoiceInputHelper");
  if (!localClass)
  {
    CLog::Log(LOGERROR, "AndroidVoiceInput: Failed to find VoiceInputHelper class");
    return false;
  }

  m_voiceInputClass = (jclass)env->NewGlobalRef(localClass);
  env->DeleteLocalRef(localClass);

  // Get method IDs
  jmethodID createMethod = env->GetStaticMethodID(
      m_voiceInputClass, "create", "()Lorg/xbmc/kodi/voice/VoiceInputHelper;");
  if (!createMethod)
  {
    CLog::Log(LOGERROR, "AndroidVoiceInput: Failed to find create method");
    return false;
  }

  // Create instance
  jobject localObj = env->CallStaticObjectMethod(m_voiceInputClass, createMethod);
  if (!localObj)
  {
    CLog::Log(LOGERROR, "AndroidVoiceInput: Failed to create VoiceInputHelper instance");
    return false;
  }

  m_speechRecognizer = env->NewGlobalRef(localObj);
  env->DeleteLocalRef(localObj);

  // Get method IDs
  m_startListeningMethod = env->GetMethodID(m_voiceInputClass, "startListening", "()V");
  m_stopListeningMethod = env->GetMethodID(m_voiceInputClass, "stopListening", "()V");
  m_cancelMethod = env->GetMethodID(m_voiceInputClass, "cancel", "()V");
  m_setLanguageMethod = env->GetMethodID(m_voiceInputClass, "setLanguage", "(Ljava/lang/String;)V");

  return (m_startListeningMethod && m_stopListeningMethod &&
          m_cancelMethod && m_setLanguageMethod);
}

bool CAndroidVoiceInput::CheckMicrophonePermission() const
{
  JNIEnv* env = xbmc_jnienv();
  if (!env)
    return false;

  jclass activityClass = env->FindClass("org/xbmc/kodi/MainActivity");
  if (!activityClass)
    return false;

  jmethodID hasPermMethod = env->GetStaticMethodID(
      activityClass, "hasMicrophonePermission", "()Z");
  if (!hasPermMethod)
  {
    env->DeleteLocalRef(activityClass);
    return false;
  }

  jboolean hasPerm = env->CallStaticBooleanMethod(activityClass, hasPermMethod);
  env->DeleteLocalRef(activityClass);

  return hasPerm == JNI_TRUE;
}

// JNI callbacks
void CAndroidVoiceInput::OnPartialResults(const std::string& text, float confidence)
{
  if (m_listener)
  {
    VoiceRecognitionResult result;
    result.text = text;
    result.confidence = confidence;
    result.languageCode = m_languageCode;
    result.isFinal = false;

    m_listener->OnPartialResult(result);
  }
}

void CAndroidVoiceInput::OnFinalResults(const std::string& text, float confidence)
{
  if (m_listener)
  {
    VoiceRecognitionResult result;
    result.text = text;
    result.confidence = confidence;
    result.languageCode = m_languageCode;
    result.isFinal = true;

    CLog::Log(LOGINFO, "AndroidVoiceInput: Final result: '{}' (confidence: {:.2f})",
              text, confidence);
    m_listener->OnFinalResult(result);
  }

  m_isListening = false;
  m_status = VoiceInputStatus::Ready;
}

void CAndroidVoiceInput::OnError(int errorCode)
{
  VoiceInputError error = VoiceInputError::RecognitionFailed;
  std::string message = "Recognition failed";

  // Map Android error codes
  switch (errorCode)
  {
    case 1: // ERROR_NETWORK_TIMEOUT
      error = VoiceInputError::Timeout;
      message = "Network timeout";
      break;
    case 2: // ERROR_NETWORK
      error = VoiceInputError::NetworkError;
      message = "Network error";
      break;
    case 6: // ERROR_SPEECH_TIMEOUT
      error = VoiceInputError::Timeout;
      message = "No speech detected";
      break;
    case 8: // ERROR_AUDIO
      error = VoiceInputError::NoMicrophone;
      message = "Audio error";
      break;
    case 9: // ERROR_INSUFFICIENT_PERMISSIONS
      error = VoiceInputError::PermissionDenied;
      message = "Insufficient permissions";
      break;
  }

  CLog::Log(LOGERROR, "AndroidVoiceInput: Error {}: {}", errorCode, message);

  m_isListening = false;
  m_status = VoiceInputStatus::Error;

  if (m_listener)
  {
    m_listener->OnError(error, message);
    m_listener->OnVoiceStatusChanged(VoiceInputStatus::Error);
  }
}

void CAndroidVoiceInput::OnReadyForSpeech()
{
  m_status = VoiceInputStatus::Listening;
  if (m_listener)
    m_listener->OnVoiceStatusChanged(VoiceInputStatus::Listening);
}

void CAndroidVoiceInput::OnBeginningOfSpeech()
{
  CLog::Log(LOGDEBUG, "AndroidVoiceInput: Beginning of speech");
}

void CAndroidVoiceInput::OnEndOfSpeech()
{
  CLog::Log(LOGDEBUG, "AndroidVoiceInput: End of speech");
  m_status = VoiceInputStatus::Processing;
  if (m_listener)
    m_listener->OnVoiceStatusChanged(VoiceInputStatus::Processing);
}

void CAndroidVoiceInput::OnRmsChanged(float rmsdB)
{
  // Convert dB to 0.0-1.0 range (approximate)
  float level = std::min(1.0f, std::max(0.0f, (rmsdB + 2.0f) / 10.0f));

  if (m_listener)
    m_listener->OnVolumeChanged(level);
}

} // namespace SEMANTIC
} // namespace KODI

#endif // TARGET_ANDROID
