/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

namespace speech
{
class ISpeechRecognitionListener;

class ISpeechRecognition
{
public:
  static std::shared_ptr<speech::ISpeechRecognition> CreateInstance();

  virtual ~ISpeechRecognition() = default;

  virtual void StartSpeechRecognition(
      const std::shared_ptr<speech::ISpeechRecognitionListener>& listener) = 0;
};

} // namespace speech
