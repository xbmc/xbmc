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

/*!
 * \file GeometryPublication.h
 * \brief How a resolved geometry goes on and comes off the wire. The only part of the geometry
 * stack that knows about CVariant.
 */

namespace KODI::VIDEO::GEOMETRY
{

//! \brief A ratio as the API states them, rounded to four places - a float widened to a double
//! serialises as sixteen digits of its own representation error, which no caller can compare.
double PublishedAspect(float aspect);

//! \brief The name this placement goes on the wire under, which is also what the API accepts.
const char* OsdPlacementName(OsdPlacement placement);

//! \return nothing when the name is not one of the two, which is how the override is given back
//! to the setting
std::optional<OsdPlacement> OsdPlacementFromName(std::string_view name);

//! \brief Do these two differ in anything SerializeEffectiveGeometry() publishes? Compared on
//! what is published, so a re-resolve onto the same answer does not move a motorised mask.
bool PublishedGeometryDiffers(const EffectiveGeometry& a, const EffectiveGeometry& b);

//! \brief Write \p geometry as the Video.ContentRect JSON-RPC type. One serialiser for the
//! library details, the player properties and the change notification alike.
void SerializeEffectiveGeometry(const EffectiveGeometry& geometry, CVariant& value);

//! \brief Write \p drawn as the Player.ScreenGeometry JSON-RPC type. Beside the content rectangle
//! rather than inside it, a library row having no screen to report.
void SerializeDrawnGeometry(const DrawnGeometry& drawn, CVariant& value);

//! \brief Write \p overrides as the Player.GetGeometry "stated" object: what was asked for, as it
//! was given rather than resolved.
void SerializeGeometryOverrides(const GeometryOverrides& overrides, CVariant& value);

/*!
 * \brief Take a Player.Geometry object onto \p overrides.
 *
 * A field that is not stated is left as it is. Zero, or null for the placement, gives one field
 * back to its setting.
 */
void ParseGeometryOverrides(const CVariant& geometry, GeometryOverrides& overrides);

} // namespace KODI::VIDEO::GEOMETRY
