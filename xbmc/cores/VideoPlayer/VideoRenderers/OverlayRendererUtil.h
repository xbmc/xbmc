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

//! Decides whether a PGS overlay's palette should be converted from
//! BT.2020 PQ to BT.709/sRGB before texture upload.
//!
//! isHDROverlay is set for both PQ and HLG video. The additional checks
//! limit conversion to PQ content on platforms without proper HDR GUI
//! compositing:
//!  - IsHdrComposite() excludes platforms that already render HDR overlays
//!    natively.
//!  - IsTransferPQ() includes only PQ/Dolby Vision output, never HLG.
bool ShouldConvertPgsPaletteToSdr(bool isHDROverlay, float& sdrWhiteNits);

//! Converts a PGS palette (CDVDOverlayImage::palette - PIXEL_A/R/G/BSHIFT-
//! packed, see PlatformDefs.h) in place from BT.2020 ST.2084 (PQ)
//! to BT.709/sRGB, scaled to the given SDR white point in nits. Alpha is
//! untouched. Operates on the whole palette (<=256 entries): PGS colour
//! information lives entirely in the palette, so converting it once here
//! is equivalent to, and far cheaper than, converting every output pixel
//! every frame in a shader.
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
