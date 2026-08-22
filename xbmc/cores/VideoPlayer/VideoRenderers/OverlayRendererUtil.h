/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <vector>

class CDVDOverlayImage;
class CDVDOverlaySpu;
class CDVDOverlaySSA;
typedef struct ass_image ASS_Image;

namespace OVERLAY
{

struct SQuad
{
  int u, v;
  unsigned char r, g, b, a;
  int x, y;
  int w, h;
};

struct SQuads
{
  int size_x{0};
  int size_y{0};
  std::vector<uint8_t> texture;
  std::vector<SQuad> quad;
};

//! How a PGS (Blu-ray bitmap) overlay's palette was handled - decided once,
//! by GetPgsHdrHandling() below, when the overlay's texture is built. See
//! that function and CDVDOverlayImage::isPgs (DVDOverlayImage.h) for why
//! this is not simply re-derived from isPgs at render time.
enum class PgsHdrHandling
{
  //! Not a PGS overlay, or HdrPgsMode::OFF, or non-PQ video: render
  //! the palette exactly as decoded.
  NONE,

  //! Palette already holds valid BT.2020 PQ code values and the GUI surface
  //! is confirmed tagged BT.2020 PQ: render unconverted.
  PQ_PASSTHROUGH,

  //! Palette was converted from BT.2020 PQ to BT.709/sRGB below,
  //! at the configured white point, before upload.
  CONVERTED_TO_SDR,
};

//! Decides how a PGS overlay's palette should be handled, from live
//! HdrPgsMode / video-transfer / GUI-tag state.
//!
//! Call once per overlay, when its texture is about to be built. This
//! reads live state, unlike the isPgs content fact. It is not re-read
//! per frame; mode/tag changes are expected to be accompanied by a new
//! overlay/rebuild. If that changes, this decision must be invalidated
//! accordingly.
//!
//! On CONVERTED_TO_SDR, sdrWhiteNits is set to the configured white point
//! in nits; left untouched otherwise.
PgsHdrHandling GetPgsHdrHandling(bool isPgs, float& sdrWhiteNits);

//! Converts a PGS palette (CDVDOverlayImage::palette - PIXEL_A/R/G/BSHIFT-
//! packed, see PlatformDefs.h) in place from BT.2020 ST.2084 (PQ)
//! to BT.709/sRGB, scaled to the given SDR white point in nits. Alpha
//! is untouched. Operates on the whole palette (<=256 entries): PGS colour
//! information lives entirely in the palette, so converting it once here is
//! equivalent to, and far cheaper than, converting every output pixel every
//! frame in a shader.
void ConvertPgsPaletteToSdr(std::vector<uint32_t>& palette, float sdrWhiteNits);

//! paletteOverride, when non-null, is used in place of o.palette - e.g.
//! a palette already converted by ConvertPgsPaletteToSdr() above. o.pixels
//! (the per-pixel palette indices) is always taken from o itself either way.
void convert_rgba(const CDVDOverlayImage& o,
                  bool mergealpha,
                  std::vector<uint32_t>& rgba,
                  const std::vector<uint32_t>* paletteOverride = nullptr);
void convert_rgba(const CDVDOverlaySpu& o,
                  bool mergealpha,
                  int& min_x,
                  int& max_x,
                  int& min_y,
                  int& max_y,
                  std::vector<uint32_t>& rgba);
bool convert_quad(ASS_Image* images, SQuads& quads, int max_x);
int GetStereoscopicDepth();

} // namespace OVERLAY
