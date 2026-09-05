/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformWasm.h"

#include "cores/AudioEngine/Sinks/AESinkWasmAudioWorklet.h"

#include <cstdlib>

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformWasm();
}

bool CPlatformWasm::InitStageOne()
{
  if (!std::getenv("HOME"))
    setenv("HOME", "/home/web_user", 1);

  if (!CPlatformPosix::InitStageOne())
    return false;

  CAESinkWasmAudioWorklet::Register();

  return true;
}
