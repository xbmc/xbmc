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
#include <cstdlib>

using namespace KODI::UTILS;

namespace KODI::VIDEO::GEOMETRY
{

CLiveGeometrySelector::CLiveGeometrySelector(const LiveSelectorParams& params) : m_params(params)
{
}

void CLiveGeometrySelector::Configure(const CRectInt& coded, float par, float atRestAspect)
{
  m_coded = coded;
  m_par = par > 0.0f ? par : 1.0f;
  m_configured = !coded.IsEmpty();
  m_atRestAspect = atRestAspect;

  m_published.reset();
  m_run.reset();
  m_shapes.clear();
  m_resync = false;
  m_degenerate = 0;
  m_implausible = 0;
  m_unconfident = 0;
}

void CLiveGeometrySelector::ResetForSeek()
{
  m_run.reset();
  m_resync = true;
}

bool CLiveGeometrySelector::SameEdges(const CRectInt& a, const CRectInt& b) const
{
  // Entry rectangles on both sides, never readings - LiveSelectorParams::jitterFloor says
  // why the tolerance must stay tight.
  return EdgesWithin(a, b, m_params.jitterFloor);
}

bool CLiveGeometrySelector::LosesPicture(const CRectInt& rect) const
{
  const CRectInt& base = m_published ? *m_published : m_coded;
  const int floor = static_cast<int>(m_params.jitterFloor);

  // Positive is inward: less picture on that edge than what is being served. Inward on one axis
  // and outward on the other still counts - 2.40 into a pillarboxed 1.33 loses the sides.
  return rect.x1 - base.x1 > floor || rect.y1 - base.y1 > floor || base.x2 - rect.x2 > floor ||
         base.y2 - rect.y2 > floor;
}

std::optional<CRectInt> CLiveGeometrySelector::Snap(const CRectInt& rect)
{
  // Judged on the display ratio, because that is what the vocabulary's entries are - a 720x576
  // DVD at 16:9 is coded at 1.25, which is on no list.
  const float ratio = static_cast<float>(rect.Width()) * m_par / static_cast<float>(rect.Height());

  // Detect, and in the room's own direction, so a live reading and a stored one name the same
  // picture the same way.
  const std::optional<AspectRatioEntry> entry =
      CAspectRatioVocabulary::Resolve(ratio, AspectRatioUse::Detect, m_atRestAspect);
  if (!entry)
  {
    // No real ratio is this shape, so it is a detector failure rather than a discovery.
    ++m_implausible;
    return std::nullopt;
  }

  return Fit(entry->ratio);
}

CRectInt CLiveGeometrySelector::Fit(float displayRatio) const
{
  // The coded frame is what the rectangle has to be expressed in, and a coded pixel is not
  // square: the ratio to fit is the display ratio taken back through the pixel aspect.
  const CRect fitted = FitAspect(displayRatio / m_par, CRect{m_coded});

  return CRectInt{static_cast<int>(std::lround(fitted.x1)), static_cast<int>(std::lround(fitted.y1)),
                  static_cast<int>(std::lround(fitted.x2)),
                  static_cast<int>(std::lround(fitted.y2))};
}

std::optional<LiveGeometryReading> CLiveGeometrySelector::Publish(const CRectInt& rect)
{
  m_published = rect;
  m_resync = false;
  m_run.reset();

  const auto known =
      std::find_if(m_shapes.begin(), m_shapes.end(),
                   [this, &rect](const CRectInt& shape) { return SameEdges(shape, rect); });
  if (known == m_shapes.end())
    m_shapes.push_back(rect);

  return LiveGeometryReading{rect, m_shapes.size() > 1};
}

std::optional<LiveGeometryReading> CLiveGeometrySelector::Feed(const DetectionResult& sample)
{
  if (!m_configured)
    return std::nullopt;

  // A cut to black carries no reading at all: a dark frame in the middle of a run neither
  // confirms it nor starts it over.
  if (sample.degenerate || sample.rect.IsEmpty())
  {
    ++m_degenerate;
    return std::nullopt;
  }

  // Without the floor a title card's bounding box can resolve to a real ratio and be served
  // for the rest of the title over a clean reading of the actual picture.
  if (sample.confidence < MIN_CORROBORATING_CONFIDENCE)
  {
    ++m_unconfident;
    return std::nullopt;
  }

  const std::optional<CRectInt> candidate = Snap(sample.rect);
  if (!candidate)
    return std::nullopt;

  // A jump. What is being served describes a part of the title playback has left, so it is not
  // merely old - it is known wrong, and this frame is the only thing that describes where we
  // landed.
  if (m_resync)
  {
    m_resync = false;
    m_run.reset();
    if (m_published && SameEdges(*candidate, *m_published))
      return std::nullopt;
    return Publish(*candidate);
  }

  // The opening frame of a stream. Nothing is established, so there is nothing to weigh this
  // against and nothing it could be wrong about.
  if (!m_published)
    return Publish(*candidate);

  if (SameEdges(*candidate, *m_published))
  {
    m_run.reset();
    return std::nullopt;
  }

  if (LosesPicture(*candidate))
  {
    if (m_run && SameEdges(m_run->rect, *candidate))
      ++m_run->frames;
    else
      m_run = Run{*candidate, 1};

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
    state = StringUtils::Format("{}x{} at {},{}", m_published->Width(), m_published->Height(),
                                m_published->x1, m_published->y1);
  else
    state = "no reading yet";

  if (m_run)
    state +=
        StringUtils::Format(", losing picture {}/{} frames", m_run->frames, m_params.narrowFrames);

  if (m_degenerate > 0 || m_implausible > 0 || m_unconfident > 0)
    state += StringUtils::Format(", dropped {} dark / {} implausible / {} unconfident",
                                 m_degenerate, m_implausible, m_unconfident);

  return state;
}

} // namespace KODI::VIDEO::GEOMETRY
