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

/*!
 * \brief How a ratio is published: its label and name, both empty when it corresponds to no entry
 *        the vocabulary holds, which is what Video.ContentRect states.
 *
 * Not Label() and Name(), which answer with the nearest entry and never reject - that suits a
 * skin needing something to show, not a client comparing what it is given.
 */
void PublishRatio(float aspect, std::string& label, std::string& name)
{
  const std::optional<AspectRatioEntry> entry{CAspectRatioVocabulary::Match(aspect)};
  label = entry ? entry->label : std::string{};
  name = entry ? entry->name : std::string{};
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

CRect PictureOnScreen(const EffectiveGeometry& geometry, const CRect& source, const CRect& dest)
{
  const CRect coded{geometry.codedFrame};
  if (geometry.displayFrame.Width() <= 0.0f || geometry.displayFrame.Height() <= 0.0f ||
      coded.Width() <= 0.0f || coded.Height() <= 0.0f || source.Width() <= 0.0f ||
      source.Height() <= 0.0f)
    return dest;

  // Only the part of the content the renderer drew: a source already cut to the content makes
  // this the whole of it, and the picture is then the whole destination.
  CRect content = SourceRect(RenderGeometryOf(geometry), coded);
  content.Intersect(source);

  // The destination is on the screen, where the turn has been applied.
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
  CRect upright = frame;

  result.displayFrame = Rotate(frame, frame, result.orientation);

  // Resolved and turned by the same steps as the answer below, so a consumer choosing a section
  // over the served rectangle is choosing between like and like. A section matching no entry is
  // still published at its own pixels: sections are publication, not resolution.
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
    result.stale = inputs.cached.state == ContentGeometryState::STALE;
  }

  if (measurement)
  {
    const CRectInt& chosen = inputs.policy == VariableGeometryPolicy::Envelope
                                 ? measurement->envelope
                                 : measurement->rect;
    const CRect candidate = ToSquarePixels(chosen, inputs.stream);

    // Resolved after pixel-aspect correction: in coded pixels an anamorphic frame sits on a
    // ratio nothing was ever shot at, close enough to an unrelated entry to pass as it. The
    // answer is the entry's rectangle, centred, so the measured offset goes with its size.
    const std::optional<AspectRatioEntry> entry = CAspectRatioVocabulary::Resolve(
        AspectOf(candidate), AspectRatioUse::Detect, inputs.atRestAspect);
    if (entry)
    {
      upright = FitAspect(entry->ratio, frame);
      result.source = measured;
      result.varies = measurement->varies;
    }
    else
    {
      // A measurement corresponding to no ratio that exists is a failed measurement rather than
      // an unusual film, so the frame stands and its staleness is not reported.
      result.stale = false;
      result.rejected = true;
    }
  }

  // Last and unconditional: nothing measured may override a declaration, and the plausibility
  // gate does not apply. The rectangle still comes from this stream's frame.
  if (inputs.declaredAspect > 0.0f)
  {
    upright = FitAspect(inputs.declaredAspect, frame);
    result.source = GeometrySource::Declared;

    // Both report on a measurement, and neither describes what is now being served. Declaring a
    // ratio over one the gate refused is what declaring is for, so leaving rejected set would
    // report the remedy as the failure.
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
  GeometryInputs detection = inputs;
  detection.declaredAspect = 0.0f;

  const EffectiveGeometry detected = ResolveEffectiveGeometry(detection);
  return detected.source == GeometrySource::Container ? 0.0f : detected.aspect;
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

  // The coded frame is the answer, so nothing is known about the content - nothing measured, or
  // a measurement rejected. Both publish an empty set rather than the frame's own ratio.
  if (geometry.source == GeometrySource::Container)
    return set;

  // varies comes from the measurement rather than the size of this set: a title whose picture
  // moves between two stretches shot at the same ratio varies and contains one ratio.
  set.varies = geometry.varies;
  set.source = geometry.source;

  for (const GeometrySection& section : geometry.sections)
  {
    // By label, which is the value a consumer compares, so two sections cannot be published
    // as different ratios while reading the same.
    const auto same = [&section](const ContentAspect& held) { return held.label == section.label; };
    if (std::none_of(set.aspects.begin(), set.aspects.end(), same))
      set.aspects.emplace_back(ContentAspect{section.label, section.name});
  }

  // Nothing retained which shapes the title was measured in: a live reading on a file with no
  // stored measurement, or a record written before the shapes were kept.
  if (set.aspects.empty())
    set.aspects.emplace_back(ContentAspect{geometry.label, geometry.name});

  return set;
}

} // namespace KODI::VIDEO::GEOMETRY
