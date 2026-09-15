/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <string>

namespace WASM_CLIPBOARD
{
/*!
 * Take any UTF-8 text pending from the last browser paste event and clear the buffer.
 * Used by CWinSystemWasmGLESContext::GetClipboardText for CGUIEditControl::OnPasteClipboard.
 *
 * The paste listener lives in platform/wasm/kodi_pre.js (main thread); it calls
 * kodi_wasm_dispatch_paste(), which must remain exported (EMSCRIPTEN_KEEPALIVE).
 */
[[nodiscard]] std::string ConsumePendingPasteText();
} // namespace WASM_CLIPBOARD
