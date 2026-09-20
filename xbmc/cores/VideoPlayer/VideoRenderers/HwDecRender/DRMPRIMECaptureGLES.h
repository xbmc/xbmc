/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CVideoBufferDRMPRIME;

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{
struct CaptureSpec;
struct CaptureResult;
} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI

//! \brief Screencap a direct-to-plane DRMPRIME video buffer.
//!
//! The video is on a DRM plane, not in the framebuffer, so it is imported as a
//! GL_TEXTURE_EXTERNAL_OES texture (the driver does YUV-to-RGB and detile),
//! drawn full-frame into a private FBO at the requested size and depth with
//! CRendererDRMPRIMEGLES::DrawTexture, read back, and tagged with the source
//! colorimetry. Render thread only, GL context current. GLES only; returns
//! false on any failure.
bool CaptureDRMPRIMEVideo(CVideoBufferDRMPRIME* buffer,
                          const KODI::RENDERING::CAPTURE::CaptureSpec& spec,
                          KODI::RENDERING::CAPTURE::CaptureResult& result);
