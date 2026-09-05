/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h"
#include "video/geometry/ContentGeometryCombiner.h"
#include "video/geometry/ContentGeometryRecord.h"
#include "video/geometry/GeometryTransforms.h"

#include <optional>
#include <string>
#include <vector>

namespace KODI::VIDEO::GEOMETRY
{

//! \brief Where an effective geometry came from, in ascending precedence.
enum class GeometrySource
{
  //! Nothing is known, so the coded frame is the answer.
  Container,

  //! A stored measurement.
  Cached,

  //! Measured during playback.
  Live,

  //! Stated by the user.
  Declared,
};

//! \brief How a title whose geometry changes partway through resolves to one rectangle.
enum class VariableGeometryPolicy
{
  //! Outer extent on each axis, so no part of the picture is ever outside the answer.
  Envelope,

  //! The geometry the title spends most of its runtime in.
  Dominant,
};

//! \brief One of the geometries a title was measured in.
struct GeometrySection
{
  CRect displayRect; //!< in the same frame as EffectiveGeometry::displayRect
  float aspect{0.0f}; //!< the upright ratio, so displayRect's transpose when rotated
  std::string label;
  std::string name;
};

//! \brief The content rectangle in force, and everything needed to interpret it.
struct EffectiveGeometry
{
  CRect displayRect; //!< the content rectangle, in display space
  CRect displayFrame; //!< the whole frame, which displayRect is contained by
  CRectInt codedFrame; //!< before pixel-aspect correction and before rotation
  float par{1.0f}; //!< width of a coded pixel in display pixels
  int orientation{0}; //!< clockwise rotation applied for display, in degrees

  float aspect{0.0f}; //!< the upright ratio of displayRect, which label names

  //! \brief What the measurement alone gave, before any declaration; zero when nothing was
  //! measured.
  float detectedAspect{0.0f};

  std::string label;
  std::string name; //!< empty when the ratio has no name
  GeometrySource source{GeometrySource::Container};

  //! \brief The title changes shape partway through, so under Dominant some picture falls
  //! outside displayRect.
  bool varies{false};

  bool stale{false}; //!< from a superseded detector, and served rather than discarded
  bool rejected{false}; //!< a measurement was gated out, so displayRect is the frame

  //! \brief Every geometry measured, in display space, dominant first. Empty when none was.
  std::vector<GeometrySection> sections;
};

//! \brief One of the distinct ratios a title contains, as it is published.
struct ContentAspect
{
  std::string label;
  std::string name;
};

//! \brief The ratios a title contains, dominant first and each once. The content's own, so a
//! variable title's envelope is not among them.
struct ContentAspectSet
{
  std::vector<ContentAspect> aspects; //!< empty when nothing was measured or declared

  bool varies{false};
  GeometrySource source{GeometrySource::Container};
};

const char* GeometrySourceName(GeometrySource source);

//! \brief The distinct ratios \p geometry establishes, dominant first.
ContentAspectSet ContentAspectsOf(const EffectiveGeometry& geometry);

//! \brief Where the picture is inside \p videoRect, the rectangle the whole frame is drawn in.
CRect PictureRect(const EffectiveGeometry& geometry, const CRect& videoRect);

//! \brief The part of \p geometry a render path reads.
RenderGeometry RenderGeometryOf(const EffectiveGeometry& geometry);

//! \brief Whether \p reading may be served over \p served. Live readings only ever widen.
bool LiveReadingWidens(float reading, float served);

//! \brief The part of \p dest showing content, \p source being the coded region drawn into it.
CRect PictureOnScreen(const RenderGeometry& geometry, const CRect& source, const CRect& dest);

//! \brief Where Kodi put the picture, in screen pixels, observed from what the renderer drew.
struct DrawnGeometry
{
  CRect picture; //!< empty while nothing is drawn
  CRect raster; //!< the area Kodi is operating in, which the picture is contained by

  bool Drawn() const { return picture.Width() > 0.0f && picture.Height() > 0.0f; }
};

//! \brief What ResolveEffectiveGeometry() answers one file from.
struct GeometryInputs
{
  StreamGeometry stream;

  float declaredAspect{0.0f}; //!< the ratio the user declared, or zero for none

  //! \brief The ratio the display rests at, which a measurement errs toward. Zero for none.
  float atRestAspect{0.0f};

  ContentGeometryLookup cached; //!< what the detection cache holds for this file
  CombinedGeometry live; //!< a playback reading, which outranks the cache when present
  bool hasLive{false};

  //! \brief The geometries measured, dominant first, in coded space. Published, never
  //! resolved from.
  std::vector<CRectInt> sections;

  VariableGeometryPolicy policy{VariableGeometryPolicy::Envelope};
};

//! \brief Which rectangle the interface is laid out against.
enum class OsdPlacement : int
{
  Raster = 0, //!< the operating area, whatever is playing
  Picture = 1, //!< the picture as it was actually drawn
};

//! \brief What an automation has told Kodi to render this playback into, each field falling
//! back to its setting when unset.
struct GeometryOverrides
{
  float rasterAspect{0.0f}; //!< the raster to operate in, or zero to use the setting
  std::optional<OsdPlacement> osdPlacement; //!< nothing to use the setting

  //! \brief The ratio to render this title into, zero for none. Nests inside the operating
  //! area rather than replacing it.
  float maintainAspect{0.0f};

  bool Any() const
  {
    return rasterAspect > 0.0f || osdPlacement.has_value() || maintainAspect > 0.0f;
  }
};

//! \brief The content rectangle in force for \p inputs. Precedence is declared, then live,
//! then cached, then the container.
EffectiveGeometry ResolveEffectiveGeometry(const GeometryInputs& inputs);

//! \brief The ratio detection alone establishes, with any declaration set aside.
float ResolveDetectedAspect(const GeometryInputs& inputs);

//! \brief The inputs to resolve while \p videoStream is decoding. A stored measurement is
//! withheld on any stream but the first.
GeometryInputs InputsForStream(const GeometryInputs& inputs, int videoStream);

} // namespace KODI::VIDEO::GEOMETRY
