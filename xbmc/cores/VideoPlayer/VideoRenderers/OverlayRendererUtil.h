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

//! True when a PQ overlay palette (isHDROverlay) must be converted to sRGB
//! for the surface it will be drawn into.
bool ShouldConvertPQPaletteToSRGB(bool isHDROverlay);

//! Converts a PGS palette (CDVDOverlayImage::palette - PIXEL_A/R/G/BSHIFT-
//! packed, see PlatformDefs.h) in place from BT.2020 ST.2084 (PQ) to
//! BT.709/sRGB, with reference white (203 nits, ITU-R BT.2408) mapped to
//! sRGB white and brighter colours scaled down to it, hue kept. Alpha is
//! untouched.
void ConvertPQPaletteToSRGB(std::vector<uint32_t>& palette);

//! paletteOverride, when non-null, is used in place of o.palette - e.g.
//! a palette already converted by ConvertPQPaletteToSRGB() above. o.pixels
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
