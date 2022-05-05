/*
 *  Copyright (C) 2012-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ISpeechRecognitionCallback.h"
#include "speech/ISpeechRecognition.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <vector>

class CJNIContext;

namespace jni
{
class CJNIXBMCSpeechRecognitionListener;
}

class CSpeechRecognitionAndroid : public ISpeechRecognitionCallback,
                                  public speech::ISpeechRecognition
{
public:
  explicit CSpeechRecognitionAndroid(const CJNIContext& context);
  ~CSpeechRecognitionAndroid() override;

  // ISpeechRecognition implementation
  void StartSpeechRecognition(
      const std::shared_ptr<speech::ISpeechRecognitionListener>& listener) override;

  // ISpeechRecognitionCallback implementation
  void SpeechRecognitionDone(jni::CJNIXBMCSpeechRecognitionListener* listener) override;

private:
  CSpeechRecognitionAndroid() = delete;

  static void RegisterSpeechRecognitionListener(void* thiz);

  const CJNIContext& m_context;
  CCriticalSection m_speechRecognitionListenersMutex;
  std::vector<std::unique_ptr<jni::CJNIXBMCSpeechRecognitionListener>> m_speechRecognitionListeners;
};
