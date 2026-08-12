/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h"
#include "video/geometry/EffectiveGeometry.h"

#include <gtest/gtest.h>

namespace KODI::VIDEO::GEOMETRY::TEST
{

//! \brief A 16:9 UHD frame with square pixels.
inline StreamGeometry Uhd(int orientation = 0)
{
  return {CRectInt{0, 0, 3840, 2160}, 16.0f / 9.0f, orientation};
}

//! \brief A PAL DVD frame carrying a 16:9 picture in 720x576 non-square pixels.
inline StreamGeometry AnamorphicPal()
{
  return {CRectInt{0, 0, 720, 576}, 16.0f / 9.0f, 0};
}

//! \brief A 2.40 operating area on a UHD display: 3840x1600, centred in 2160.
inline const CRect ScopeRaster{0.0f, 280.0f, 3840.0f, 1880.0f};

inline ContentGeometryLookup Cached(const CRectInt& rect,
                                    const CRectInt& envelope,
                                    bool varies = false,
                                    ContentGeometryState state = ContentGeometryState::VALID)
{
  ContentGeometryLookup lookup;
  lookup.state = state;
  lookup.record.rect = rect;
  lookup.record.envelope = envelope;
  lookup.record.varies = varies;
  lookup.record.hasReading = true;
  return lookup;
}

//! \brief A measured 2.40 letterbox on an HD frame: the 1920x800 body at {0,140}, with the
//! envelope defaulted to the rectangle and everything else at the record's own defaults.
inline ContentGeometryRecord ScopeHdRecord(const CRectInt& envelope = CRectInt{0, 140, 1920, 940})
{
  ContentGeometryRecord record;
  record.coded = CRectInt{0, 0, 1920, 1080};
  record.rect = CRectInt{0, 140, 1920, 940};
  record.envelope = envelope;
  record.displayAspect = 16.0f / 9.0f;
  record.hasReading = true;
  return record;
}

//! \brief A 2.40 measurement on a UHD frame, which most of the resolver tests start from.
inline GeometryInputs ScopeCachedUhd()
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached = Cached(CRectInt{0, 280, 3840, 1880}, CRectInt{0, 280, 3840, 1880});
  return inputs;
}

//! \brief A 3.20 measurement on a UHD frame - further from every entry than the tolerance,
//! which is what the resolver refuses.
inline GeometryInputs RefusedCachedUhd()
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached = Cached(CRectInt{0, 480, 3840, 1680}, CRectInt{0, 480, 3840, 1680});
  return inputs;
}

//! \brief A 2.35 measurement on the anamorphic PAL frame: 720x436 coded pixels whose display
//! ratio only comes out right after pixel-aspect correction.
inline GeometryInputs ScopeCachedPal(const CRectInt& envelope = CRectInt{0, 70, 720, 506},
                                     bool varies = false)
{
  GeometryInputs inputs;
  inputs.stream = AnamorphicPal();
  inputs.cached = Cached(CRectInt{0, 70, 720, 506}, envelope, varies);
  return inputs;
}

inline void ExpectRect(const CRect& actual, float x1, float y1, float x2, float y2)
{
  EXPECT_NEAR(x1, actual.x1, 0.01f);
  EXPECT_NEAR(y1, actual.y1, 0.01f);
  EXPECT_NEAR(x2, actual.x2, 0.01f);
  EXPECT_NEAR(y2, actual.y2, 0.01f);
}

inline void ExpectRect(const CRectInt& actual, int x1, int y1, int x2, int y2)
{
  EXPECT_EQ(x1, actual.x1);
  EXPECT_EQ(y1, actual.y1);
  EXPECT_EQ(x2, actual.x2);
  EXPECT_EQ(y2, actual.y2);
}

} // namespace KODI::VIDEO::GEOMETRY::TEST
