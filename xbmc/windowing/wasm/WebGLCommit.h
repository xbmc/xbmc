/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

extern "C"
{
  // Blits Emscripten's offscreen framebuffer to the canvas on the browser main
  // thread, after every GL call queued before it. Implemented in webgl_commit.js.
  // Returns 1 on success, 0 when no proxied context is current.
  int wasm_webgl_commit_frame(void);
}
