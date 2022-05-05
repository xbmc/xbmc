/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

namespace speech
{
class ISpeechRecognitionListener
{
public:
  virtual ~ISpeechRecognitionListener() = default;

  virtual void OnReadyForSpeech() {}
  virtual void OnError(int recognitionError) = 0;
  virtual void OnResults(const std::vector<std::string>& results) = 0;
};

} // namespace speech
