/*
 *  Copyright (C) 2012-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SpeechRecognitionStub.h"

std::shared_ptr<speech::ISpeechRecognition> speech::ISpeechRecognition::CreateInstance()
{
  // speech recognition not implemented
  return {};
}
