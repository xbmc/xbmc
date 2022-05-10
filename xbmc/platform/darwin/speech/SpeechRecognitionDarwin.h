/*
 *  Copyright (C) 2012-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "speech/ISpeechRecognition.h"

#include <memory>

struct SpeechRecognitionDarwinImpl;

class CSpeechRecognitionDarwin : public speech::ISpeechRecognition
{
public:
  CSpeechRecognitionDarwin();
  ~CSpeechRecognitionDarwin() override;

  // ISpeechRecognition implementation
  void StartSpeechRecognition(
      const std::shared_ptr<speech::ISpeechRecognitionListener>& listener) override;

  void OnRecognitionDone(speech::ISpeechRecognitionListener* listener);

private:
  std::unique_ptr<SpeechRecognitionDarwinImpl> m_impl;
};
