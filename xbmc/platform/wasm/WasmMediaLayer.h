/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// The <video> elements the media backends play in, under the GUI canvas; implemented in
// media_layer.js.

extern "C"
{
  // Where the video goes, in canvas pixels; an empty rectangle means the whole canvas.
  void wasm_media_set_display_rect(int x, int y, int width, int height);
}
