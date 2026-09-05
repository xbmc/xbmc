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

/*!
 * \file EffectiveGeometry.h
 * \brief Resolves which content rectangle is in force for a file.
 */

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

  //! Stated by the user, which pins.
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

  //! \brief The ratio the content was shot at, which is what label names. displayRect's own
  //! ratio, transposed when rotated.
  float aspect{0.0f};

  std::string label;
  std::string name; //!< empty when the ratio has no name
  GeometrySource source{GeometrySource::Container};

  //! \brief One rectangle does not describe the whole title. Under
  //! VariableGeometryPolicy::Dominant picture falls outside displayRect during the minority
  //! sections.
  bool varies{false};

  bool stale{false}; //!< from a superseded detector, and served rather than discarded
  bool rejected{false}; //!< a measurement was gated out, so displayRect is the frame

  //! \brief Every geometry the title was measured in, in display space, dominant first. Empty
  //! when nothing was measured.
  std::vector<GeometrySection> sections;
};

//! \brief One of the distinct ratios a title contains, as it is published.
struct ContentAspect
{
  std::string label;
  std::string name;
};

//! \brief The ratios a title contains, dominant first, with no ratio appearing twice. The
//! content's own, so the envelope a variable title is served at is not one of them.
struct ContentAspectSet
{
  std::vector<ContentAspect> aspects; //!< empty when nothing was measured or declared

  bool varies{false};
  GeometrySource source{GeometrySource::Container};
};

//! \brief The name \p source is published under.
const char* GeometrySourceName(GeometrySource source);

/*!
 * \brief The distinct ratios \p geometry establishes, dominant first.
 *
 * \return an empty set when the coded frame is the answer - nothing measured, or a measurement
 *         the plausibility gate rejected
 */
ContentAspectSet ContentAspectsOf(const EffectiveGeometry& geometry);

/*!
 * \brief Where the picture is on the screen, as a fraction of \p videoRect.
 *
 * \param videoRect where the whole video frame is drawn, in screen coordinates
 * \return \p videoRect itself when the frame is not known
 */
CRect PictureRect(const EffectiveGeometry& geometry, const CRect& videoRect);

//! \brief The part of \p geometry a render path reads.
RenderGeometry RenderGeometryOf(const EffectiveGeometry& geometry);

/*!
 * \brief May a live reading at \p reading be served, when the room has already been told
 *        \p served?
 *
 * Live readings only ever widen. A title opening on a 16:9 ident before settling at 2.35 has to
 * open the masking; a scene composed narrower than the title it is in must not bring it back,
 * because a director framing a shot is not the film changing shape, and a room that followed one
 * would rearrange itself around the cutting. Narrowing is a decision only a measurement of the
 * whole title can make.
 *
 * \param reading the reading's display ratio
 * \param served the display ratio already being served: zero before anything is, and seeded from
 *        a stored measurement's widest shape when there is one, so the opening scene of a
 *        measured title cannot move the room either
 */
bool LiveReadingWidens(float reading, float served);

/*!
 * \brief Where the content is on the screen for a renderer that samples only a region of the
 *        coded frame. PictureRect() answers for one that draws the whole frame.
 *
 * \param source the region of the coded frame drawn, in coded pixels
 * \param dest where that region is drawn, in screen coordinates
 * \return the part of \p dest showing content; \p dest itself when nothing is known
 */
CRect PictureOnScreen(const EffectiveGeometry& geometry, const CRect& source, const CRect& dest);

//! \brief Where Kodi put the picture, in screen pixels, observed from what the renderer drew.
struct DrawnGeometry
{
  CRect picture; //!< empty while nothing is drawn
  CRect raster; //!< the area Kodi is operating in, which the picture is contained by

  bool Drawn() const { return picture.Width() > 0.0f && picture.Height() > 0.0f; }
};

//! \brief Everything the resolver needs to answer for one file.
struct GeometryInputs
{
  StreamGeometry stream;

  float declaredAspect{0.0f}; //!< the ratio the user declared, or zero for none

  //! \brief The shape the room rests at, the direction a measurement errs in. Zero means no
  //! preference, and the entry nearest the measurement wins.
  float atRestAspect{0.0f};

  ContentGeometryLookup cached; //!< what the detection cache holds for this file
  CombinedGeometry live; //!< a playback reading, which outranks the cache when present
  bool hasLive{false};

  //! \brief The geometries the title was measured in, dominant first, in coded space. Published
  //! for a consumer that implements its own policy, never resolved from.
  std::vector<CRectInt> sections;

  VariableGeometryPolicy policy{VariableGeometryPolicy::Envelope};
};

//! \brief Which rectangle the interface is laid out against. The values are the setting's.
enum class OsdPlacement : int
{
  Raster = 0, //!< the operating area, whatever is playing
  Picture = 1, //!< the picture as it was actually drawn
};

/*!
 * \brief What an automation system has told Kodi to render this playback into. Each field falls
 *        back to the corresponding setting when unset.
 *
 * Not GeometryInputs::declaredAspect, which says what the content is; the two may disagree.
 */
struct GeometryOverrides
{
  float rasterAspect{0.0f}; //!< the raster to operate in, or zero to use the setting
  std::optional<OsdPlacement> osdPlacement; //!< nothing to use the setting

  //! \brief The ratio to render this title into, or zero for none. Nests inside the operating
  //! area rather than replacing it - see MaintainedRect().
  float maintainAspect{0.0f};

  bool Any() const
  {
    return rasterAspect > 0.0f || osdPlacement.has_value() || maintainAspect > 0.0f;
  }
};

/*!
 * \brief The content rectangle in force for \p inputs.
 *
 * Precedence is declared, then live, then cached, then the container. A measurement is served at
 * the vocabulary entry it identifies through FitAspect() rather than at its own pixels, erring
 * toward GeometryInputs::atRestAspect between two entries; one within tolerance of no entry at
 * all is rejected. Every path that cannot establish the content falls through to the coded frame.
 */
EffectiveGeometry ResolveEffectiveGeometry(const GeometryInputs& inputs);

/*!
 * \brief The ratio detection alone establishes for \p inputs, with any declaration set aside.
 *
 * \return zero when the coded frame would be the answer, which is not the same as a measurement
 *         that agreed with the container
 */
float ResolveDetectedAspect(const GeometryInputs& inputs);

/*!
 * \brief The inputs to resolve while \p videoStream is the one decoding.
 *
 * A stored measurement describes the first video stream, so on any other one it is withheld
 * rather than cleared. A live reading passes through whichever stream is decoding.
 */
GeometryInputs InputsForStream(const GeometryInputs& inputs, int videoStream);

} // namespace KODI::VIDEO::GEOMETRY
