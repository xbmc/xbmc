/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GeometryPublication.h"

#include <cmath>
#include <utility>

namespace KODI::VIDEO::GEOMETRY
{

namespace
{

//! \brief The wire shape every published rectangle takes: origin and size, four keys.
void WriteRect(CVariant& value, const CRect& rect)
{
  value["x"] = rect.x1;
  value["y"] = rect.y1;
  value["width"] = rect.Width();
  value["height"] = rect.Height();
}

} // unnamed namespace

double PublishedAspect(float aspect)
{
  return std::round(static_cast<double>(aspect) * 10000.0) / 10000.0;
}

const char* OsdPlacementName(OsdPlacement placement)
{
  return placement == OsdPlacement::Picture ? "picture" : "raster";
}

std::optional<OsdPlacement> OsdPlacementFromName(std::string_view name)
{
  if (name == "picture")
    return OsdPlacement::Picture;

  if (name == "raster")
    return OsdPlacement::Raster;

  return std::nullopt;
}

bool PublishedGeometryDiffers(const EffectiveGeometry& a, const EffectiveGeometry& b)
{
  return a.displayRect != b.displayRect || a.displayFrame != b.displayFrame ||
         a.source != b.source || a.varies != b.varies || a.stale != b.stale || a.label != b.label;
}

void SerializeEffectiveGeometry(const EffectiveGeometry& geometry, CVariant& value)
{
  value = CVariant{CVariant::VariantTypeObject};

  // Omitted when there is no frame behind it. The ratio is always present.
  if (geometry.displayFrame.Width() > 0.0f && geometry.displayFrame.Height() > 0.0f)
  {
    WriteRect(value, geometry.displayRect);
    value["framewidth"] = geometry.displayFrame.Width();
    value["frameheight"] = geometry.displayFrame.Height();
  }

  value["aspect"] = PublishedAspect(geometry.aspect);
  value["label"] = geometry.label;
  value["name"] = geometry.name;
  value["varies"] = geometry.varies;
  value["stale"] = geometry.stale;

  value["rejected"] = geometry.rejected;

  // Always written, empty included.
  value["sections"] = CVariant{CVariant::VariantTypeArray};
  for (const GeometrySection& section : geometry.sections)
  {
    CVariant published{CVariant::VariantTypeObject};
    WriteRect(published, section.displayRect);
    published["aspect"] = PublishedAspect(section.aspect);
    published["label"] = section.label;
    published["name"] = section.name;
    value["sections"].push_back(std::move(published));
  }

  value["source"] = GeometrySourceName(geometry.source);
}

void SerializeDrawnGeometry(const DrawnGeometry& drawn, CVariant& value)
{
  value = CVariant{CVariant::VariantTypeObject};

  WriteRect(value["picture"], drawn.picture);
  WriteRect(value["raster"], drawn.raster);
}

void SerializeGeometryOverrides(const GeometryOverrides& overrides, CVariant& value)
{
  value["raster"] = PublishedAspect(overrides.rasterAspect);
  value["maintain"] = PublishedAspect(overrides.maintainAspect);
  if (overrides.osdPlacement)
    value["osdplacement"] = OsdPlacementName(*overrides.osdPlacement);
  else
    value["osdplacement"] = CVariant{CVariant::VariantTypeNull};
}

void ParseGeometryOverrides(const CVariant& geometry, GeometryOverrides& overrides)
{
  if (!geometry.isObject())
    return;

  // "Not stated" is null: JSON-RPC fills omitted properties in from the service description.
  const CVariant& raster = geometry["raster"];
  if (!raster.isNull())
    overrides.rasterAspect = raster.asFloat();

  const CVariant& maintain = geometry["maintain"];
  if (!maintain.isNull())
    overrides.maintainAspect = maintain.asFloat();

  const CVariant& placement = geometry["osdplacement"];
  if (!placement.isNull())
    overrides.osdPlacement = OsdPlacementFromName(placement.asString());
}

} // namespace KODI::VIDEO::GEOMETRY
