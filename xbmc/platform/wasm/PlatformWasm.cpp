/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "PlatformWasm.h"

#include "cores/AudioEngine/Sinks/AESinkWasmAudioWorklet.h"
#include "filesystem/wasm/WasmFilesystem.h"
#include "windowing/wasm/WinSystemWasmGLESContext.h"

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

  CWinSystemWasmGLESContext::Register();
  CAESinkWasmAudioWorklet::Register();
  KODI::PLATFORM::WASM::EnsureVirtualFilesystem();

  return true;
}

void CPlatformWasm::DeinitStageOne()
{
  CPlatformPosix::DeinitStageOne();
}
