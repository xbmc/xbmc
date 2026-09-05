/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SampledGeometry.h"

#include "XBDateTime.h"

namespace KODI::VIDEO::GEOMETRY
{

ContentGeometryRecord MakeContentGeometryRecord(const SampledGeometry& scan,
                                                const FileIdentity& identity,
                                                const CDateTime& computed)
{
  ContentGeometryRecord record;
  record.identity = identity;
  record.computed = computed;

  if (!scan.succeeded)
  {
    record.outcome = ContentGeometryOutcome::Failed;

    // Stated rather than left unsaid, so that storing this clears whatever an earlier
    // measurement of the same file left behind: nothing about it survives a scan that could
    // not read it.
    record.details = std::string{};
    return record;
  }

  record.outcome = ContentGeometryOutcome::Measured;
  record.coded = scan.coded;
  record.rect = scan.combined.rect;

  // The combiner leaves the envelope empty only when it had no cluster to take an extent of,
  // in which case the rectangle it did commit to is the whole answer.
  record.envelope = scan.combined.envelope.IsEmpty() ? scan.combined.rect : scan.combined.envelope;

  record.displayAspect = scan.displayAspect;
  record.varies = scan.combined.varies;
  record.hasReading = scan.combined.hasReading;
  record.confidence = scan.combined.share;

  // The clusters' rectangles alone, kept where a consumer can read them without the rest of
  // what produced them.
  record.sections.reserve(scan.combined.clusters.size());
  for (const GeometryCluster& cluster : scan.combined.clusters)
    record.sections.push_back(cluster.rect);

  ContentGeometryDetails details;
  details.detector = CONTENT_GEOMETRY_DETECTOR;
  details.combining = scan.combining;
  details.sampling = scan.sampling;
  details.samples = scan.samples;
  details.clusters = scan.combined.clusters;
  details.usable = scan.combined.usable;
  details.discarded = scan.combined.discarded;
  details.unreadable = scan.unreadable;
  record.details = EncodeContentGeometryDetails(details);

  return record;
}

} // namespace KODI::VIDEO::GEOMETRY
