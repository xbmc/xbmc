/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDFileGeometry.h"

#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDDecodeSession.h"
#include "DVDFileInfo.h"
#include "DVDStreamInfo.h"
#include "FileItem.h"
#include "URL.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/geometry/ContentBarDetector.h"
#include "video/geometry/FrameReduction.h"
#include "video/geometry/FrameSampling.h"
#include "video/geometry/GeometryTransforms.h"

#include <algorithm>
#include <chrono>
#include <optional>
#include <string>
#include <vector>

extern "C"
{
#include <libavutil/pixdesc.h>
}

using namespace KODI::VIDEO::GEOMETRY;

static ColorRange GeometryColorRange(const VideoPicture& picture, const CDVDStreamInfo& hint)
{
  if (hint.colorRange == AVCOL_RANGE_JPEG)
    return ColorRange::Full;
  if (hint.colorRange == AVCOL_RANGE_MPEG)
    return ColorRange::Limited;
  return picture.color_range ? ColorRange::Full : ColorRange::Unspecified;
}

// The detector needs planar, little-endian, low-aligned samples; anything else is refused.
bool CDVDFileGeometry::BuildGeometryFrameRef(const VideoPicture& picture,
                                             const CDVDStreamInfo& hint,
                                             FrameRef& frame)
{
  if (!picture.videoBuffer || picture.iWidth == 0 || picture.iHeight == 0)
    return false;

  const AVPixFmtDescriptor* description = av_pix_fmt_desc_get(picture.pixelFormat);
  if (!description || description->nb_components < 3)
    return false;

  if (!(description->flags & AV_PIX_FMT_FLAG_PLANAR) || (description->flags & AV_PIX_FMT_FLAG_BE) ||
      (description->flags & AV_PIX_FMT_FLAG_PAL))
    return false;

  const unsigned int depth = description->comp[0].depth;
  if (depth < 8 || depth > 16)
    return false;

  if (description->log2_chroma_w == 1 && description->log2_chroma_h == 1)
    frame.subsampling = ChromaSubsampling::YUV420;
  else if (description->log2_chroma_w == 1 && description->log2_chroma_h == 0)
    frame.subsampling = ChromaSubsampling::YUV422;
  else if (description->log2_chroma_w == 0 && description->log2_chroma_h == 0)
    frame.subsampling = ChromaSubsampling::YUV444;
  else
    return false;

  uint8_t* planes[YuvImage::MAX_PLANES] = {};
  int strides[YuvImage::MAX_PLANES] = {};
  picture.videoBuffer->GetPlanes(planes);
  picture.videoBuffer->GetStrides(strides);
  if (!planes[0] || strides[0] == 0)
    return false;

  frame.width = picture.iWidth;
  frame.height = picture.iHeight;
  frame.bitDepth = depth;
  frame.y = {planes[0], strides[0]};
  if (planes[1] && planes[2])
  {
    frame.u = {planes[1], strides[1]};
    frame.v = {planes[2], strides[2]};
  }

  frame.range = GeometryColorRange(picture, hint);

  frame.roi = StereoViewRect(picture.stereoMode, picture.iWidth, picture.iHeight);
  return true;
}

bool CDVDFileGeometry::BuildGeometryFrameRef(const ReducedFrame& reduction,
                                             const VideoPicture& picture,
                                             const CDVDStreamInfo& hint,
                                             FrameRef& frame)
{
  if (reduction.width == 0 || reduction.height == 0)
    return false;

  const size_t lumaSize = static_cast<size_t>(reduction.width) * reduction.height;
  if (reduction.y.size() < lumaSize || reduction.u.size() < lumaSize / 4 ||
      reduction.v.size() < lumaSize / 4)
    return false;

  frame.width = reduction.width;
  frame.height = reduction.height;
  frame.bitDepth = 8;
  frame.subsampling = ChromaSubsampling::YUV420;
  frame.y = {reduction.y.data(), static_cast<int>(reduction.width)};
  frame.u = {reduction.u.data(), static_cast<int>(reduction.width / 2)};
  frame.v = {reduction.v.data(), static_cast<int>(reduction.width / 2)};

  frame.range = GeometryColorRange(picture, hint);

  frame.roi = StereoViewRect(picture.stereoMode, reduction.width, reduction.height);
  return true;
}

//! \brief One scan of one open file: the session it reads through, the parameters, and what
//! sampling accumulates across passes.
struct GeometrySampleRun
{
  DVDDecodeSession& session;
  unsigned int perPoint;
  const std::function<bool()>& cancelled;
  const std::string& redactPath;
  const std::string& logName;
  SampledGeometry& scan;
  std::vector<double> visited{};
  int packetsTried{0};

  //! \brief One pass over \p schedule: seek to each point and read its pictures into the scan.
  void Sample(const std::vector<double>& schedule);
};

void GeometrySampleRun::Sample(const std::vector<double>& schedule)
{
  for (const double offset : schedule)
  {
    if (cancelled && cancelled())
    {
      scan.cancelled = true;
      return;
    }

    visited.push_back(offset);
    for (unsigned int repeat = 0; repeat < perPoint; ++repeat)
    {
      VideoPicture picture = {};
      const auto position = static_cast<int64_t>(offset * 1000.0);
      if (!SeekAndDecodePictureAt(*session.demuxer, *session.codec, session.videoStream, position,
                                  redactPath, picture, packetsTried))
      {
        ++scan.unreadable;
        CLog::LogF(LOGDEBUG, "no picture decoded at {:.1f}s in {}", offset, redactPath);
        continue;
      }

      FrameRef frame;
      if (!CDVDFileGeometry::BuildGeometryFrameRef(picture, session.hint, frame))
      {
        ++scan.unreadable;
        CLog::LogF(LOGDEBUG, "picture at {:.1f}s is in no format the detector reads ({}) in {}",
                   offset, picture.pixelFormat, redactPath);
        continue;
      }

      if (!scan.succeeded)
      {
        scan.succeeded = true;
        scan.coded = MeasuredFrameRect(picture.stereoMode, frame.width, frame.height);
      }

      const DetectionResult detected = DetectContentRect(frame);
      scan.samples.push_back({detected.rect, detected.confidence, detected.degenerate, offset});

      CLog::LogF(LOGDEBUG, "[{}] {:.1f}s: {}x{} at {},{} confidence {:.4f}{}", logName, offset,
                 detected.rect.Width(), detected.rect.Height(), detected.rect.x1, detected.rect.y1,
                 detected.confidence,
                 detected.degenerate ? " (degenerate, carries no reading)" : "");
    }
  }
}

SampledGeometry CDVDFileGeometry::ExtractContentGeometry(const CFileItem& fileItem,
                                                         const SamplingParams& sampling,
                                                         const CombinerParams& combining,
                                                         const std::function<bool()>& cancelled)
{
  SampledGeometry scan;
  scan.sampling = sampling;
  scan.combining = combining;

  if (!CDVDFileInfo::CanExtract(fileItem))
    return scan;

  const std::string redactPath = CURL::GetRedacted(fileItem.GetPath());
  const auto start = std::chrono::steady_clock::now();

  std::optional<DVDDecodeSession> session =
      OpenDVDDecodeSession(fileItem, CODEC_FORCE_SOFTWARE | CODEC_EXPORT_FILM_GRAIN, redactPath);
  if (!session)
    return scan;

  scan.displayAspect = static_cast<float>(session->hint.aspect);

  const double duration = session->demuxer->GetStreamLength() / 1000.0;
  const std::vector<double> offsets = SampleOffsets(duration, sampling);
  if (offsets.empty())
  {
    CLog::LogF(LOGDEBUG, "no usable duration for {}", redactPath);
    return scan;
  }

  const std::string logName = URIUtils::GetFileName(redactPath);

  const auto [windowStart, windowEnd] = SampleWindow(duration, sampling);
  CLog::LogF(LOGDEBUG, "sampling {} points across {:.1f}-{:.1f}s of {:.1f}s in {}", offsets.size(),
             windowStart, windowEnd, duration, redactPath);

  GeometrySampleRun run{
      *session, std::max(1u, sampling.picturesPerPoint), cancelled, redactPath, logName, scan};

  run.Sample(offsets);
  if (scan.cancelled || !scan.succeeded)
    return scan;

  scan.combined = CombineGeometrySamples(scan.samples, scan.coded, combining);

  if (ShouldEscalate(scan.combined.clusters.size(), scan.combined.usable, scan.combined.discarded,
                     scan.combined.unexplainedShapes, sampling))
  {
    SamplingParams denser = sampling;
    denser.points = sampling.escalatedPoints;
    const std::vector<double> extra =
        UnsampledOffsets(SampleOffsets(duration, denser), run.visited);

    CLog::LogF(LOGDEBUG, "first pass inconclusive for {}, adding {} points", redactPath,
               extra.size());

    run.Sample(extra);
    if (scan.cancelled)
      return scan;

    scan.combined = CombineGeometrySamples(scan.samples, scan.coded, combining);
  }

  for (size_t i = 0; i < scan.combined.clusters.size(); ++i)
  {
    const GeometryCluster& cluster = scan.combined.clusters[i];
    CLog::LogF(LOGDEBUG, "cluster {}: {}x{} at {},{} from {} samples, weight {:.4f}{}", i,
               cluster.rect.Width(), cluster.rect.Height(), cluster.rect.x1, cluster.rect.y1,
               cluster.samples, cluster.weight, i == 0 ? " (chosen)" : "");
  }

  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);

  CLog::LogF(LOGDEBUG,
             "sampled {} points ({} readings: {} usable, {} discarded, {} unreadable) in {} ms "
             "from <{}>: {}x{}{}",
             run.visited.size(), scan.samples.size(), scan.combined.usable, scan.combined.discarded,
             scan.unreadable, elapsed.count(), redactPath, scan.combined.rect.Width(),
             scan.combined.rect.Height(), scan.combined.varies ? ", varies" : "");

  if (scan.combined.unexplainedShapes > 0)
  {
    CLog::LogF(LOGDEBUG, "{} reading(s) of a shape the answer does not account for in {}",
               scan.combined.unexplainedShapes, redactPath);
  }

  return scan;
}
