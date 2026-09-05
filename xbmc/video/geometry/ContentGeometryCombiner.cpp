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
  CRectInt centre;
  float weight{0.0f};
  float bestConfidence{0.0f};
  std::vector<CRectInt> members;
};

std::array<int, 4> Edges(const CRectInt& rect)
{
  return {rect.x1, rect.y1, rect.x2, rect.y2};
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
      values.push_back(Edges(member)[component]);
    std::sort(values.begin(), values.end());
    centre[component] = values[values.size() / 2];
  }

  return {centre[0], centre[1], centre[2], centre[3]};
}

//! \brief Do two rectangles describe different shapes, rather than the same one measured twice?
//! Compared as ratios: two readings of one boundary differ by pixels where two shapes do not.
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
    // A degenerate frame - a cut to black - is no reading rather than a very narrow one, and
    // combining it as narrow is how a mask closes onto picture.
    if (sample.degenerate)
    {
      ++result.discarded;
      continue;
    }

    auto match =
        std::find_if(clusters.begin(), clusters.end(), [&](const WorkingCluster& cluster)
                     { return EdgesWithin(cluster.centre, sample.rect, params.tolerance); });

    if (match == clusters.end())
      clusters.push_back({sample.rect, sample.confidence, sample.confidence, {sample.rect}});
    else
    {
      match->weight += sample.confidence;
      match->bestConfidence = std::max(match->bestConfidence, sample.confidence);
      match->members.push_back(sample.rect);
    }
  }

  // Where some of the title scored, the parts that did not are the suspect ones and are
  // dropped. Where nothing scored anywhere, a rectangle that kept recurring is the only
  // evidence there is.
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

  for (WorkingCluster& cluster : clusters)
  {
    cluster.centre = MedianRect(cluster.members);
    result.usable += static_cast<unsigned int>(cluster.members.size());
  }

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
    result.clusters.push_back(
        {cluster.centre, static_cast<unsigned int>(cluster.members.size()), cluster.weight});
  }

  result.hasReading = true;
  result.rect = result.clusters.front().rect;

  // Every corroborated cluster, including ones too small to make the title count as varying: a
  // section that occurred is picture however often it occurred. Unioning can only widen.
  result.envelope = CRectInt{};
  for (const GeometryCluster& cluster : result.clusters)
    result.envelope.Union(cluster.rect);
  // Falls back to counts when nothing scored, so that a stationary reading does not report a
  // zero share and read as though it had no support.
  result.share = totalWeight > 0.0f ? result.clusters.front().weight / totalWeight
                                    : static_cast<float>(result.clusters.front().samples) /
                                          static_cast<float>(result.usable);

  // A reading can be too thin to trust as an answer and still too different to be noise: the
  // corroboration rules above test repetition, the wrong test for an entirely different shape.
  // Counted rather than acted on - ShouldEscalate() is what looks.
  for (const CRectInt& shape : rejected)
  {
    if (DiffersInShape(shape, result.rect, params.distinctAspectShare))
      ++result.unexplainedShapes;
  }

  // Counts, not weight - see CombinedGeometry::varies for why this must not be weighted.
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
