/*
 *  Copyright (C) 2012-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SpeechRecognitionAndroid.h"

#include "speech/ISpeechRecognitionListener.h"
#include "speech/SpeechRecognitionErrors.h"
#include "utils/log.h"

#include "platform/android/activity/JNIMainActivity.h"
#include "platform/android/activity/XBMCApp.h"
#include "platform/android/speech/SpeechRecognitionListenerAndroid.h"

#include <mutex>

#include <androidjni/Context.h>
#include <androidjni/Intent.h>
#include <androidjni/IntentFilter.h>
#include <androidjni/PackageManager.h>
#include <androidjni/RecognizerIntent.h>
#include <androidjni/SpeechRecognizer.h>
#include <jni.h>

std::shared_ptr<speech::ISpeechRecognition> speech::ISpeechRecognition::CreateInstance()
{
  return std::make_shared<CSpeechRecognitionAndroid>(CXBMCApp::Get());
}

CSpeechRecognitionAndroid::CSpeechRecognitionAndroid(const CJNIContext& context)
  : m_context(context)
{
}

CSpeechRecognitionAndroid::~CSpeechRecognitionAndroid()
{
}

void CSpeechRecognitionAndroid::StartSpeechRecognition(
    const std::shared_ptr<speech::ISpeechRecognitionListener>& listener)
{
  if (CJNISpeechRecognizer::isRecognitionAvailable(m_context))
  {
    if (CJNIContext::checkCallingOrSelfPermission("android.permission.RECORD_AUDIO") ==
        CJNIPackageManager::PERMISSION_GRANTED)
    {
      std::unique_lock<CCriticalSection> lock(m_speechRecognitionListenersMutex);
      m_speechRecognitionListeners.emplace_back(
          std::make_unique<CSpeechRecognitionListenerAndroid>(listener, *this));
      lock.unlock();

      // speech recognizer init must be called from the main thread:
      // https://developer.android.com/reference/android/speech/SpeechRecognizer
      jni::CJNIMainActivity::runNativeOnUiThread(RegisterSpeechRecognitionListener, this);
    }
    else
    {
      CLog::LogF(LOGERROR, "Permission RECORD_AUDIO is not granted");
      listener->OnError(speech::RecognitionError::INSUFFICIENT_PERMISSIONS);
    }
  }
  else
  {
    CLog::LogF(LOGERROR, "Speech recognition service is not available");
    listener->OnError(speech::RecognitionError::SERVICE_NOT_AVAILABLE);
  }
}

void CSpeechRecognitionAndroid::RegisterSpeechRecognitionListener(void* thiz)
{
  CSpeechRecognitionAndroid* sra = static_cast<CSpeechRecognitionAndroid*>(thiz);

  CJNISpeechRecognizer speechRecognizer =
      CJNISpeechRecognizer::createSpeechRecognizer(sra->m_context);

  std::unique_lock<CCriticalSection> lock(sra->m_speechRecognitionListenersMutex);
  speechRecognizer.setRecognitionListener(*(sra->m_speechRecognitionListeners.back()));
  lock.unlock();

  CJNIIntent intent = CJNIIntent(CJNIRecognizerIntent::ACTION_RECOGNIZE_SPEECH);
  intent.putExtra(CJNIRecognizerIntent::EXTRA_LANGUAGE_MODEL,
                  CJNIRecognizerIntent::LANGUAGE_MODEL_FREE_FORM);

  speechRecognizer.startListening(intent);
}

void CSpeechRecognitionAndroid::SpeechRecognitionDone(
    jni::CJNIXBMCSpeechRecognitionListener* listener)
{
  std::unique_lock<CCriticalSection> lock(m_speechRecognitionListenersMutex);
  for (auto it = m_speechRecognitionListeners.begin(); it != m_speechRecognitionListeners.end();
       ++it)
  {
    if ((*it).get() == listener)
    {
      m_speechRecognitionListeners.erase(it);
      break;
    }
  }
}
