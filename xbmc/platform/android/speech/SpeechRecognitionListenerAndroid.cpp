/*
 *  Copyright (C) 2012-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SpeechRecognitionListenerAndroid.h"

#include "speech/ISpeechRecognitionListener.h"
#include "speech/SpeechRecognitionErrors.h"
#include "utils/log.h"

#include "platform/android/speech/ISpeechRecognitionCallback.h"

#include <androidjni/ArrayList.h>
#include <androidjni/Bundle.h>
#include <androidjni/SpeechRecognizer.h>

CSpeechRecognitionListenerAndroid::CSpeechRecognitionListenerAndroid(
    const std::shared_ptr<speech::ISpeechRecognitionListener>& listener,
    ISpeechRecognitionCallback& callback)
  : m_listener(listener), m_callback(callback)
{
}

CSpeechRecognitionListenerAndroid::~CSpeechRecognitionListenerAndroid()
{
}

void CSpeechRecognitionListenerAndroid::onReadyForSpeech(CJNIBundle bundle)
{
  if (m_listener)
    m_listener->OnReadyForSpeech();
  else
    CLog::LogF(LOGERROR, "No speech recognition listener");
}

void CSpeechRecognitionListenerAndroid::onError(int i)
{
  if (m_listener)
  {
    int recognitionError;
    if (i == CJNISpeechRecognizer::ERROR_AUDIO)
      recognitionError = speech::RecognitionError::AUDIO;
    else if (i == CJNISpeechRecognizer::ERROR_CLIENT)
      recognitionError = speech::RecognitionError::CLIENT;
    else if (i == CJNISpeechRecognizer::ERROR_INSUFFICIENT_PERMISSIONS)
      recognitionError = speech::RecognitionError::INSUFFICIENT_PERMISSIONS;
    else if (i == CJNISpeechRecognizer::ERROR_NETWORK)
      recognitionError = speech::RecognitionError::NETWORK;
    else if (i == CJNISpeechRecognizer::ERROR_NETWORK_TIMEOUT)
      recognitionError = speech::RecognitionError::NETWORK_TIMEOUT;
    else if (i == CJNISpeechRecognizer::ERROR_NO_MATCH)
      recognitionError = speech::RecognitionError::NO_MATCH;
    else if (i == CJNISpeechRecognizer::ERROR_RECOGNIZER_BUSY)
      recognitionError = speech::RecognitionError::RECOGNIZER_BUSY;
    else if (i == CJNISpeechRecognizer::ERROR_SERVER)
      recognitionError = speech::RecognitionError::SERVER;
    else if (i == CJNISpeechRecognizer::ERROR_SPEECH_TIMEOUT)
      recognitionError = speech::RecognitionError::SPEECH_TIMEOUT;
    else
      recognitionError = speech::RecognitionError::UNKNOWN;

    m_listener->OnError(recognitionError);
  }
  else
    CLog::LogF(LOGERROR, "No speech recognition listener");

  m_callback.SpeechRecognitionDone(this);
}

void CSpeechRecognitionListenerAndroid::onResults(CJNIBundle bundle)
{
  if (m_listener)
  {
    CJNIArrayList<std::string> r =
        bundle.getStringArrayList(CJNISpeechRecognizer::RESULTS_RECOGNITION);

    std::vector<std::string> results;
    results.reserve(r.size());
    for (int i = 0; i < r.size(); ++i)
      results.emplace_back(r.get(i));

    m_listener->OnResults(results);
  }
  else
    CLog::LogF(LOGERROR, "No speech recognition listener");

  m_callback.SpeechRecognitionDone(this);
}
