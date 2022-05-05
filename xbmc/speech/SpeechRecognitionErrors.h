/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace speech
{
namespace RecognitionError
{
static constexpr int UNKNOWN = 1;
static constexpr int NETWORK_TIMEOUT = 2;
static constexpr int NETWORK = 3;
static constexpr int AUDIO = 4;
static constexpr int SERVER = 5;
static constexpr int CLIENT = 6;
static constexpr int SPEECH_TIMEOUT = 7;
static constexpr int NO_MATCH = 8;
static constexpr int RECOGNIZER_BUSY = 9;
static constexpr int INSUFFICIENT_PERMISSIONS = 10;
static constexpr int SERVICE_NOT_AVAILABLE = 11;

} // namespace RecognitionError
} // namespace speech
