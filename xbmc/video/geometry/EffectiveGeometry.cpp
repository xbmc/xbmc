/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EffectiveGeometry.h"

#include "utils/AspectRatioVocabulary.h"

#include <algorithm>
#include <optional>
#include <utility>

using namespace KODI::UTILS;

namespace KODI::VIDEO::GEOMETRY
{

namespace
{

//! \brief One measurement, whatever produced it.
struct Measurement
{
  CRectInt rect;
  CRectInt envelope;
  bool varies{false};
};

//! \brief What the measurement in force resolves to, before any declaration is applied.
struct ResolvedMeasurement
{
  CRect upright; //!< the frame itself when nothing was measured or the measurement was rejected
  GeometrySource source{GeometrySource::Container};
  bool varies{false};
  bool stale{false};
  bool rejected{false};
};

//! \brief A ratio's label and name, both empty when it matches no entry. Not Label() and
//! Name(), which answer with the nearest entry and never reject.
void PublishRatio(float aspect, std::string& label, std::string& name)
{
  const std::optional<AspectRatioEntry> entry{CAspectRatioVocabulary::Match(aspect)};
  label = entry ? entry->label : std::string{};
  name = entry ? entry->name : std::string{};
}

/*!
 * \brief Resolve the live reading, else the stored one, against the vocabulary. The sections
 * are published rather than resolved from, so this does not touch them.
 * \param frame the coded frame with its pixel aspect undone, which \p upright is a region of
 */
ResolvedMeasurement ResolveMeasurement(const GeometryInputs& inputs, const CRect& frame)
{
  ResolvedMeasurement resolved;
  resolved.upright = frame;

  std::optional<Measurement> measurement;
  GeometrySource measured = GeometrySource::Container;

  if (inputs.hasLive && inputs.live.hasReading)
  {
    measurement = Measurement{inputs.live.rect, inputs.live.envelope, inputs.live.varies};
    measured = GeometrySource::Live;
  }
  else if (inputs.cached.HasRecord() && inputs.cached.record.hasReading)
  {
    const ContentGeometryRecord& record = inputs.cached.record;
    measurement = Measurement{record.rect, record.envelope, record.varies};
    measured = GeometrySource::Cached;
    resolved.stale = inputs.cached.state == ContentGeometryState::STALE;
  }

  if (!measurement)
    return resolved;

  const CRectInt& chosen =
      inputs.policy == VariableGeometryPolicy::Envelope ? measurement->envelope : measurement->rect;
  const CRect candidate = ToSquarePixels(chosen, inputs.stream);

  // The answer is the matched ratio's rectangle, centred, after pixel-aspect correction.
  const std::optional<AspectRatioEntry> entry = CAspectRatioVocabulary::Resolve(
      AspectOf(candidate), AspectRatioUse::Detect, inputs.atRestAspect);
  if (!entry)
  {
    // A measurement matching no real ratio is a failed measurement, so the frame stands.
    resolved.stale = false;
    resolved.rejected = true;
    return resolved;
  }

  resolved.upright = FitAspect(entry->ratio, frame);
  resolved.source = measured;
  resolved.varies = measurement->varies;
  return resolved;
}

} // unnamed namespace

CRect PictureRect(const EffectiveGeometry& geometry, const CRect& videoRect)
{
  const CRect& frame = geometry.displayFrame;
  if (frame.Width() <= 0.0f || frame.Height() <= 0.0f)
    return videoRect;

  return FromFraction(FractionOf(geometry.displayRect, frame), videoRect);
}

RenderGeometry RenderGeometryOf(const EffectiveGeometry& geometry)
{
  return {geometry.codedFrame, geometry.displayFrame, geometry.displayRect,
          geometry.aspect,     geometry.par,          geometry.orientation};
}

bool LiveReadingWidens(float reading, float served)
{
  // Wide enough to be a different ratio rather than the detector wandering by a row.
  constexpr float WIDEN_EPSILON = 0.02f;

  return reading > served + WIDEN_EPSILON;
}

CRect PictureOnScreen(const RenderGeometry& geometry, const CRect& source, const CRect& dest)
{
  const CRect coded{geometry.codedFrame};
  if (geometry.displayFrame.Width() <= 0.0f || geometry.displayFrame.Height() <= 0.0f ||
      coded.Width() <= 0.0f || coded.Height() <= 0.0f || source.Width() <= 0.0f ||
      source.Height() <= 0.0f)
    return dest;

  // Only the part of the content the renderer drew.
  CRect content = SourceRect(geometry, coded);
  content.Intersect(source);

  const CRect unit{0.0f, 0.0f, 1.0f, 1.0f};
  const CRect fraction =
      Rotate(FractionOf(content, source), unit, NormaliseRotation(geometry.orientation));

  return FromFraction(fraction, dest);
}

GeometryInputs InputsForStream(const GeometryInputs& inputs, int videoStream)
{
  if (videoStream <= 0)
    return inputs;

  GeometryInputs withheld{inputs};
  withheld.cached = {};
  withheld.sections.clear();
  return withheld;
}

EffectiveGeometry ResolveEffectiveGeometry(const GeometryInputs& inputs)
{
  EffectiveGeometry result;
  result.codedFrame = inputs.stream.coded;
  result.par = PixelAspectRatio(inputs.stream);
  result.orientation = NormaliseRotation(inputs.stream.orientation);

  // Everything is decided upright; the turn is applied once, at the end.
  const CRect frame = ToSquarePixels(inputs.stream.coded, inputs.stream);

  result.displayFrame = Rotate(frame, frame, result.orientation);

  // A section matching no entry is still published at its own pixels.
  result.sections.reserve(inputs.sections.size());
  for (const CRectInt& section : inputs.sections)
  {
    const CRect square = ToSquarePixels(section, inputs.stream);
    GeometrySection published;
    const std::optional<AspectRatioEntry> entry = CAspectRatioVocabulary::Resolve(
        AspectOf(square), AspectRatioUse::Detect, inputs.atRestAspect);
    if (entry)
    {
      published.aspect = entry->ratio;
      published.label = entry->label;
      published.name = entry->name;
      published.displayRect = Rotate(FitAspect(entry->ratio, frame), frame, result.orientation);
    }
    else
    {
      published.aspect = AspectOf(square);
      PublishRatio(published.aspect, published.label, published.name);
      published.displayRect = Rotate(square, frame, result.orientation);
    }
    result.sections.push_back(std::move(published));
  }

  const ResolvedMeasurement measurement = ResolveMeasurement(inputs, frame);
  CRect upright = measurement.upright;
  result.source = measurement.source;
  result.varies = measurement.varies;
  result.stale = measurement.stale;
  result.rejected = measurement.rejected;

  result.detectedAspect = result.source == GeometrySource::Container ? 0.0f : AspectOf(upright);

  // Nothing measured may override a declaration, and the plausibility gate does not apply.
  if (inputs.declaredAspect > 0.0f)
  {
    upright = FitAspect(inputs.declaredAspect, frame);
    result.source = GeometrySource::Declared;

    result.stale = false;
    result.rejected = false;
  }

  result.aspect = AspectOf(upright);
  PublishRatio(result.aspect, result.label, result.name);
  result.displayRect = Rotate(upright, frame, result.orientation);
  return result;
}

float ResolveDetectedAspect(const GeometryInputs& inputs)
{
  // Only the measurement decides this, so the sections and the labelling are not resolved.
  const CRect frame = ToSquarePixels(inputs.stream.coded, inputs.stream);
  const ResolvedMeasurement measurement = ResolveMeasurement(inputs, frame);

  return measurement.source == GeometrySource::Container ? 0.0f : AspectOf(measurement.upright);
}

const char* GeometrySourceName(GeometrySource source)
{
  switch (source)
  {
    case GeometrySource::Declared:
      return "declared";
    case GeometrySource::Live:
      return "live";
    case GeometrySource::Cached:
      return "cached";
    case GeometrySource::Container:
    default:
      return "container";
  }
}

ContentAspectSet ContentAspectsOf(const EffectiveGeometry& geometry)
{
  ContentAspectSet set;

  if (geometry.source == GeometrySource::Container)
    return set;

  // varies comes from the measurement rather than the size of this set: a title can move
  // between two stretches shot at the same ratio.
  set.varies = geometry.varies;
  set.source = geometry.source;

  for (const GeometrySection& section : geometry.sections)
  {
    const auto same = [&section](const ContentAspect& held) { return held.label == section.label; };
    if (std::none_of(set.aspects.begin(), set.aspects.end(), same))
      set.aspects.emplace_back(ContentAspect{section.label, section.name});
  }

  // Nothing retained which shapes the title was measured in.
  if (set.aspects.empty())
    set.aspects.emplace_back(ContentAspect{geometry.label, geometry.name});

  return set;
}

} // namespace KODI::VIDEO::GEOMETRY
