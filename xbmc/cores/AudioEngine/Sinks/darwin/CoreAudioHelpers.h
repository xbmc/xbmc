/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

#include <AudioToolbox/AudioToolbox.h>

// Helper Functions
std::string GetError(OSStatus error);
const char* StreamDescriptionToString(AudioStreamBasicDescription desc, std::string &str);
