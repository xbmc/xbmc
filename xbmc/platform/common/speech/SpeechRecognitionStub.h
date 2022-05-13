/*
 *  Copyright (C) 2012-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "speech/ISpeechRecognition.h"

// This is a stub for all platforms without a speech recognition implementation
// Saves us from feature/platform ifdeffery.
class CSpeechRecognitionStub : public speech::ISpeechRecognition
{
public:
  void StartSpeechRecognition(
      const std::shared_ptr<speech::ISpeechRecognitionListener>& listener) override
  {
  }
};
