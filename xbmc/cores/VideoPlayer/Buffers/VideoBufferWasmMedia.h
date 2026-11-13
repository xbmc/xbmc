/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/VideoBuffer.h"

/*!
 * \brief Marks pictures that a media backend shows in its own <video>; it carries no image.
 */
class CVideoBufferWasmMedia final : public CVideoBuffer
{
public:
  CVideoBufferWasmMedia() : CVideoBuffer(0) {}
  AVPixelFormat GetFormat() override { return AV_PIX_FMT_NONE; }
};
