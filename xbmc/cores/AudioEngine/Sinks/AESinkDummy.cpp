/*
 *  Copyright (C) 2010-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AESinkDummy.h"
#include "utils/log.h"

CAESinkDummy::CAESinkDummy() = default;
CAESinkDummy::~CAESinkDummy() = default;

const char* CAESinkDummy::GetName()
{
  return "dummy";
}

bool CAESinkDummy::Initialize(AEAudioFormat &format, std::string &device)
{
  (void)format;
  device = "dummy";
  CLog::Log(LOGWARNING, "AESinkDummy: Initialized dummy audio sink. No audio will be output.");
  return true;
}

void CAESinkDummy::Deinitialize() {}

double CAESinkDummy::GetCacheTotal()
{
  return 0.0;
}

double CAESinkDummy::GetLatency()
{
  return 0.0;
}

void CAESinkDummy::Drain() {}

unsigned int CAESinkDummy::AddPackets(uint8_t ** /*data*/, unsigned int frames, unsigned int /*offset*/)
{
  return frames;
}

void CAESinkDummy::GetDelay(AEDelayStatus& status)
{
  status.SetDelay(0.0);
}

bool CAESinkDummy::HasVolume()
{
  return false;
}

void CAESinkDummy::SetVolume(float /*volume*/)
{
}
