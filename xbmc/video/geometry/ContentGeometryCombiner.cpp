/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContentGeometryCombiner.h"

#include "video/geometry/GeometryTransforms.h"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace KODI::VIDEO::GEOMETRY
{

namespace
{

//! \brief A cluster while it is still being built, keeping its members for re-centring.
struct WorkingCluster
{
  //! \brief The first member's rectangle, which later samples are matched against. Where the
  //! cluster actually sits is the median of its members, taken once it is closed.
  CRectInt seed;

  float weight{0.0f};
  float bestConfidence{0.0f};
  std::vector<CRectInt> members;
};

//! \brief One edge of a rectangle by component: x1, y1, x2, then y2.
int Edge(const CRectInt& rect, size_t component)
{
  switch (component)
  {
    case 0:
      return rect.x1;
    case 1:
      return rect.y1;
    case 2:
      return rect.x2;
    default:
      return rect.y2;
  }
}

//! \brief Component-wise median, so one early member cannot define where a cluster sits.
CRectInt MedianRect(const std::vector<CRectInt>& members)
{
  std::array<int, 4> centre{};
  std::vector<int> values;
  values.reserve(members.size());

  for (size_t component = 0; component < centre.size(); ++component)
  {
    values.clear();
    for (const CRectInt& member : members)
      values.push_back(Edge(member, component));
    std::sort(values.begin(), values.end());
    centre[component] = values[values.size() / 2];
  }

  return {centre[0], centre[1], centre[2], centre[3]};
}

//! \brief Whether two rectangles describe different shapes rather than one measured twice.
//! Compared as ratios: two readings of one boundary differ by pixels.
bool DiffersInShape(const CRectInt& a, const CRectInt& b, float share)
{
  if (a.Width() <= 0 || a.Height() <= 0 || b.Width() <= 0 || b.Height() <= 0)
    return false;

  const float lhs = static_cast<float>(a.Width()) / static_cast<float>(a.Height());
  const float rhs = static_cast<float>(b.Width()) / static_cast<float>(b.Height());
  return std::abs(lhs - rhs) / std::min(lhs, rhs) > share;
}

} // unnamed namespace

CombinedGeometry CombineGeometrySamples(std::span<const GeometrySample> samples,
                                        const CRectInt& coded,
                                        const CombinerParams& params)
{
  CombinedGeometry result;
  result.rect = coded;
  result.envelope = coded;

  std::vector<WorkingCluster> clusters;

  for (const GeometrySample& sample : samples)
  {
    // A degenerate frame is no reading rather than a very narrow one.
    if (sample.degenerate)
    {
      ++result.discarded;
      continue;
    }

    auto match = std::find_if(clusters.begin(), clusters.end(), [&](const WorkingCluster& cluster)
                              { return EdgesWithin(cluster.seed, sample.rect, params.tolerance); });

    if (match == clusters.end())
      clusters.push_back({sample.rect, sample.confidence, sample.confidence, {sample.rect}});
    else
    {
      match->weight += sample.confidence;
      match->bestConfidence = std::max(match->bestConfidence, sample.confidence);
      match->members.push_back(sample.rect);
    }
  }

  // Where some of the title scored, the parts that did not are dropped.
  const bool anyConfident =
      std::any_of(clusters.begin(), clusters.end(), [&](const WorkingCluster& cluster)
                  { return cluster.bestConfidence >= params.minConfidence; });

  std::vector<CRectInt> rejected;
  std::erase_if(clusters,
                [&](const WorkingCluster& cluster)
                {
                  const bool corroborated =
                      anyConfident ? cluster.bestConfidence >= params.minConfidence
                                   : cluster.members.size() >= params.minStationarySamples;
                  if (corroborated)
                    return false;

                  result.discarded += static_cast<unsigned int>(cluster.members.size());
                  rejected.push_back(MedianRect(cluster.members));
                  return true;
                });

  if (clusters.empty())
    return result; // no reading; rect stays at the coded frame, which is never narrower

  for (const WorkingCluster& cluster : clusters)
    result.usable += static_cast<unsigned int>(cluster.members.size());

  std::sort(clusters.begin(), clusters.end(),
            [](const WorkingCluster& a, const WorkingCluster& b)
            {
              if (a.weight != b.weight)
                return a.weight > b.weight;
              return a.members.size() > b.members.size();
            });

  float totalWeight = 0.0f;
  for (const WorkingCluster& cluster : clusters)
  {
    totalWeight += cluster.weight;
    result.clusters.push_back({MedianRect(cluster.members),
                               static_cast<unsigned int>(cluster.members.size()), cluster.weight});
  }

  result.hasReading = true;
  result.rect = result.clusters.front().rect;

  // Every corroborated cluster, including ones too small to make the title count as varying.
  result.envelope = CRectInt{};
  for (const GeometryCluster& cluster : result.clusters)
    result.envelope.Union(cluster.rect);
  // Falls back to counts when nothing scored.
  result.share = totalWeight > 0.0f ? result.clusters.front().weight / totalWeight
                                    : static_cast<float>(result.clusters.front().samples) /
                                          static_cast<float>(result.usable);

  // Counted rather than acted on; ShouldEscalate() reads it.
  for (const CRectInt& shape : rejected)
  {
    if (DiffersInShape(shape, result.rect, params.distinctAspectShare))
      ++result.unexplainedShapes;
  }

  // Counts, not weight.
  const unsigned int counted = result.usable;
  result.varies =
      std::any_of(result.clusters.begin() + 1, result.clusters.end(),
                  [&](const GeometryCluster& cluster)
                  {
                    return cluster.samples >= params.minRivalSamples &&
                           static_cast<float>(cluster.samples) / static_cast<float>(counted) >=
                               params.variesShare;
                  });

  return result;
}

} // namespace KODI::VIDEO::GEOMETRY
