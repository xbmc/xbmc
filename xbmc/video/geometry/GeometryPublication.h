/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Variant.h"
#include "video/geometry/EffectiveGeometry.h"

#include <optional>
#include <string_view>

namespace KODI::VIDEO::GEOMETRY
{

//! \brief A ratio as the API states them, rounded to four places so a caller can compare it.
double PublishedAspect(float aspect);

//! \brief The name this placement goes on the wire under, which is also what the API accepts.
const char* OsdPlacementName(OsdPlacement placement);

//! \brief The placement \p name states, nothing when it is neither.
std::optional<OsdPlacement> OsdPlacementFromName(std::string_view name);

//! \brief Whether these two differ in anything SerializeEffectiveGeometry() publishes.
bool PublishedGeometryDiffers(const EffectiveGeometry& a, const EffectiveGeometry& b);

//! \brief Write \p geometry as the Video.ContentRect JSON-RPC type.
void SerializeEffectiveGeometry(const EffectiveGeometry& geometry, CVariant& value);

//! \brief Write \p drawn as the Player.ScreenGeometry JSON-RPC type.
void SerializeDrawnGeometry(const DrawnGeometry& drawn, CVariant& value);

//! \brief Write \p overrides as the Player.GetGeometry "stated" object: what was asked for,
//! not what it resolved to.
void SerializeGeometryOverrides(const GeometryOverrides& overrides, CVariant& value);

//! \brief Take a Player.Geometry object onto \p overrides. An unstated field is left alone;
//! zero, or null for the placement, gives one back to its setting.
void ParseGeometryOverrides(const CVariant& geometry, GeometryOverrides& overrides);

} // namespace KODI::VIDEO::GEOMETRY
