/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "application/AppEnvironment.h"
#include "application/Application.h"
#include "application/ApplicationPowerHandling.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <chrono>

#include <emscripten.h>

// Built only for CORE_SYSTEM_NAME==wasm (see application/CMakeLists.txt).
void CApplication::WasmRunIteration()
{
  if (m_bStop)
  {
    emscripten_cancel_main_loop();
    Cleanup();
    CAppEnvironment::TearDown();
    CLog::Log(LOGINFO, "Exiting the application (WASM)...");
    return;
  }

  static std::chrono::time_point<std::chrono::steady_clock> lastFrameTime =
      std::chrono::steady_clock::now();
  std::chrono::milliseconds frameTime;
  const unsigned int noRenderFrameTime = 15;

  Process();

  bool renderGUI = GetComponent<CApplicationPowerHandling>()->GetRenderGUI();
  if (!m_bStop)
    FrameMove(true, renderGUI);

  if (renderGUI && !m_bStop)
    Render();
  else if (!renderGUI)
  {
    auto now = std::chrono::steady_clock::now();
    frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime);
    if (frameTime.count() < noRenderFrameTime)
    {
      KODI::TIME::Sleep(std::chrono::milliseconds(noRenderFrameTime - frameTime.count()));
    }
  }
  lastFrameTime = std::chrono::steady_clock::now();
}
