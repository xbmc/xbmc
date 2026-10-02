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

#include <utility>
#include <vector>

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

//! \brief A stored measurement holding \p aspects, dominant first.
inline ContentGeometryLookup Cached(std::vector<float> aspects,
                                    ContentGeometryState state = ContentGeometryState::VALID)
{
  ContentGeometryLookup lookup;
  lookup.state = state;
  lookup.record.aspects = std::move(aspects);
  return lookup;
}

//! \brief A measured 2.40 title, unless \p aspects says otherwise.
inline ContentGeometryRecord ScopeRecord(std::vector<float> aspects = {2.40f})
{
  ContentGeometryRecord record;
  record.aspects = std::move(aspects);
  return record;
}

//! \brief A 2.40 measurement on a UHD frame, which most of the resolver tests start from.
inline GeometryInputs ScopeCachedUhd()
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached = Cached({2.40f});
  return inputs;
}

//! \brief A 3.20 measurement on a UHD frame - further from every entry than the tolerance,
//! which is what the resolver refuses.
inline GeometryInputs RefusedCachedUhd()
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached = Cached({3.20f});
  return inputs;
}

//! \brief A 2.35 measurement on the anamorphic PAL frame, whose 720x576 coded pixels are not
//! square.
inline GeometryInputs ScopeCachedPal(std::vector<float> aspects = {2.35f})
{
  GeometryInputs inputs;
  inputs.stream = AnamorphicPal();
  inputs.cached = Cached(std::move(aspects));
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
