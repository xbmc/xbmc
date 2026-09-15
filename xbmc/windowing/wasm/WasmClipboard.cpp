/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "WasmClipboard.h"

#include "ServiceBroker.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"

#include <mutex>
#include <string>

#include <emscripten.h>

namespace
{
std::mutex g_mutex;
std::string g_pending;
} // namespace

// Browser paste listener is registered in kodi_pre.js (main thread only). Pthreads have no document.

extern "C" EMSCRIPTEN_KEEPALIVE void kodi_wasm_dispatch_paste(const char* utf8)
{
  std::string text = utf8 ? utf8 : "";
  {
    std::lock_guard lock(g_mutex);
    g_pending = std::move(text);
  }
  auto* action = new CAction(ACTION_PASTE);
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, action);
}

std::string WASM_CLIPBOARD::ConsumePendingPasteText()
{
  std::lock_guard lock(g_mutex);
  return std::move(g_pending);
}
