/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LiveGeometrySelector.h"

#include "utils/AspectRatioVocabulary.h"
#include "utils/StringUtils.h"
#include "video/geometry/ContentGeometryCombiner.h"
#include "video/geometry/GeometryTransforms.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace KODI::UTILS;

namespace KODI::VIDEO::GEOMETRY
{

namespace
{

//! \brief The tallest frame read as SD, which is where a broadcast flags the wrong shape.
constexpr int SD_MAX_HEIGHT = 576;

//! \brief How close a frame has to be to 16:9 or 4:3 to count as that SD shape, in log space.
constexpr float SD_SHAPE_TOLERANCE = 0.01f;

//! \brief How far a picture read as the other SD shape may be from an entry, in log space.
//! Wider than the vocabulary's own: a misflagged channel's letterbox is rarely exact.
constexpr float OTHER_SHAPE_TOLERANCE = 0.05f;

} // unnamed namespace

CLiveGeometrySelector::CLiveGeometrySelector(const LiveSelectorParams& params) : m_params(params)
{
}

void CLiveGeometrySelector::Configure(const CRectInt& coded, float par, float atRestAspect)
{
  m_coded = coded;
  m_par = par > 0.0f ? par : 1.0f;
  m_configured = !coded.IsEmpty();
  m_atRestAspect = atRestAspect;

  m_otherShape = 0.0f;
  if (m_configured && coded.Height() <= SD_MAX_HEIGHT)
  {
    const float shown =
        static_cast<float>(coded.Width()) * m_par / static_cast<float>(coded.Height());
    if (CAspectRatioVocabulary::Distance(shown, 16.0f / 9.0f) <= SD_SHAPE_TOLERANCE)
      m_otherShape = 0.75f;
    else if (CAspectRatioVocabulary::Distance(shown, 4.0f / 3.0f) <= SD_SHAPE_TOLERANCE)
      m_otherShape = 4.0f / 3.0f;
  }

  m_published.reset();
  m_publishedCorrection = 1.0f;
  m_run.reset();
  m_shapes.clear();
  m_resync = false;
  m_degenerate = 0;
  m_implausible = 0;
  m_unconfident = 0;
  m_inset = 0;
}

void CLiveGeometrySelector::ResetForSeek()
{
  m_run.reset();
  m_resync = true;
}

bool CLiveGeometrySelector::SameEdges(const CRectInt& a, const CRectInt& b) const
{
  // Entry rectangles on both sides, never readings.
  return EdgesWithin(a, b, m_params.jitterFloor);
}

bool CLiveGeometrySelector::LosesPicture(const CRectInt& rect) const
{
  const CRectInt& base = m_published ? *m_published : m_coded;
  const int floor = static_cast<int>(m_params.jitterFloor);

  // Positive is inward: less picture on that edge than is being served. Inward on one axis and
  // outward on the other still counts.
  return rect.x1 - base.x1 > floor || rect.y1 - base.y1 > floor || base.x2 - rect.x2 > floor ||
         base.y2 - rect.y2 > floor;
}

bool CLiveGeometrySelector::GainsPicture(const CRectInt& rect) const
{
  const CRectInt& base = m_published ? *m_published : m_coded;
  const int floor = static_cast<int>(m_params.jitterFloor);

  // Positive is outward: more picture on that edge than what is being served.
  return base.x1 - rect.x1 > floor || base.y1 - rect.y1 > floor || rect.x2 - base.x2 > floor ||
         rect.y2 - base.y2 > floor;
}

std::optional<CLiveGeometrySelector::Snapped> CLiveGeometrySelector::Snap(const CRectInt& rect)
{
  // Judged on the display ratio.
  const float ratio = static_cast<float>(rect.Width()) * m_par / static_cast<float>(rect.Height());

  // The ratio alone: nothing here reads a label or a name.
  if (const std::optional<float> resolved =
          CAspectRatioVocabulary::ResolveRatio(ratio, AspectRatioUse::Detect, m_atRestAspect))
    return Snapped{Fit(*resolved, m_par), 1.0f};

  // A picture of no real ratio in an SD frame is the frame flagged with the wrong shape when,
  // read as the other one, it is near a real ratio.
  if (m_otherShape > 0.0f)
  {
    if (const std::optional<float> resolved = CAspectRatioVocabulary::ResolveRatio(
            ratio * m_otherShape, AspectRatioUse::Detect, m_atRestAspect, OTHER_SHAPE_TOLERANCE))
      return Snapped{Fit(*resolved, m_par * m_otherShape), m_otherShape};
  }

  // No real ratio is this shape, so it is a detector failure rather than a discovery.
  ++m_implausible;
  return std::nullopt;
}

CRectInt CLiveGeometrySelector::Fit(float displayRatio, float par) const
{
  // The rectangle is expressed in coded pixels, so the ratio to fit is the display ratio
  // taken back through the pixel aspect.
  const CRect fitted = FitAspect(displayRatio / par, CRect{m_coded});

  return CRectInt{
      static_cast<int>(std::lround(fitted.x1)), static_cast<int>(std::lround(fitted.y1)),
      static_cast<int>(std::lround(fitted.x2)), static_cast<int>(std::lround(fitted.y2))};
}

LiveGeometryReading CLiveGeometrySelector::Publish(const Snapped& snapped)
{
  const CRectInt& rect = snapped.rect;
  m_published = rect;
  m_publishedCorrection = snapped.frameCorrection;
  m_resync = false;
  m_run.reset();

  const auto known =
      std::find_if(m_shapes.begin(), m_shapes.end(),
                   [this, &rect](const CRectInt& shape) { return SameEdges(shape, rect); });
  if (known == m_shapes.end())
    m_shapes.push_back(rect);

  return {rect, m_shapes.size() > 1, snapped.frameCorrection};
}

std::optional<LiveGeometryReading> CLiveGeometrySelector::Feed(const DetectionResult& sample)
{
  if (!m_configured)
    return std::nullopt;

  // A cut to black carries no reading: it neither confirms a run nor starts it over.
  if (sample.degenerate || sample.rect.IsEmpty())
  {
    ++m_degenerate;
    return std::nullopt;
  }

  if (sample.confidence < MIN_CORROBORATING_CONFIDENCE)
  {
    ++m_unconfident;
    return std::nullopt;
  }

  const std::optional<Snapped> candidate = Snap(sample.rect);
  if (!candidate)
    return std::nullopt;

  // Widening counts only when the reading itself widens.
  if (m_published && GainsPicture(candidate->rect) && !GainsPicture(sample.rect))
  {
    ++m_inset;
    return std::nullopt;
  }

  const bool alreadyServed = m_published && SameEdges(candidate->rect, *m_published) &&
                             candidate->frameCorrection == m_publishedCorrection;

  if (m_resync)
  {
    m_resync = false;
    m_run.reset();
    if (alreadyServed)
      return std::nullopt;
    return Publish(*candidate);
  }

  // The opening frame of a stream, with nothing established to weigh it against.
  if (!m_published)
    return Publish(*candidate);

  if (alreadyServed)
  {
    m_run.reset();
    return std::nullopt;
  }

  if (LosesPicture(candidate->rect))
  {
    if (m_run && SameEdges(m_run->rect, candidate->rect))
      ++m_run->frames;
    else
      m_run = Run{candidate->rect, 1};

    if (m_run->frames < m_params.narrowFrames)
      return std::nullopt;
  }

  return Publish(*candidate);
}

std::string CLiveGeometrySelector::Describe() const
{
  if (!m_configured)
    return "waiting for a frame";

  std::string state;
  if (m_published)
    state = StringUtils::Format("{}x{} at {},{}{}", m_published->Width(), m_published->Height(),
                                m_published->x1, m_published->y1,
                                m_publishedCorrection != 1.0f ? " in the other SD shape" : "");
  else
    state = "no reading yet";

  if (m_run)
    state +=
        StringUtils::Format(", losing picture {}/{} frames", m_run->frames, m_params.narrowFrames);

  if (m_degenerate > 0 || m_implausible > 0 || m_unconfident > 0 || m_inset > 0)
    state += StringUtils::Format(", dropped {} dark / {} implausible / {} unconfident / {} inset",
                                 m_degenerate, m_implausible, m_unconfident, m_inset);

  return state;
}

} // namespace KODI::VIDEO::GEOMETRY
