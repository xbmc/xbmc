/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/geometry/SampledGeometry.h"

#include <functional>

class CFileItem;
class CDVDStreamInfo;
struct VideoPicture;

namespace KODI::VIDEO::GEOMETRY
{
struct FrameRef;
struct ReducedFrame;
} // namespace KODI::VIDEO::GEOMETRY

/*!
 * \brief The player-side content geometry sampler: decodes a file at several points and
 * describes each picture to the detector.
 */
class CDVDFileGeometry
{
public:
  /*!
   * \brief Sample a file at several points and work out its true picture rectangle.
   *
   * Opens the item once and walks it, the cost of a sample point being the seek rather than the
   * decode. Gated on CDVDFileInfo::CanExtract(). Decodes in software with film grain synthesis
   * disabled, synthesised grain over clean coded bars being what the detector must not see.
   *
   * \param cancelled asked between sample positions, so abandoning returns within a seek. The
   *                  scan then reports itself cancelled and must not be stored.
   */
  static KODI::VIDEO::GEOMETRY::SampledGeometry ExtractContentGeometry(
      const CFileItem& fileItem,
      const KODI::VIDEO::GEOMETRY::SamplingParams& sampling = {},
      const KODI::VIDEO::GEOMETRY::CombinerParams& combining = {},
      const std::function<bool()>& cancelled = {});

  /*!
   * \brief Describe a decoded picture to the content geometry detector, for the scan-time
   *        sampler and the live monitor alike.
   *
   * Reads the layout from the pixel format descriptor rather than switching on known formats, so
   * an unfamiliar one is described correctly or refused. Fails for a picture whose buffer exposes
   * no planes.
   *
   * \param hint the stream, for the color range the picture itself has already collapsed
   */
  static bool BuildGeometryFrameRef(const VideoPicture& picture,
                                    const CDVDStreamInfo& hint,
                                    KODI::VIDEO::GEOMETRY::FrameRef& frame);

  /*!
   * \brief Point a FrameRef at a reduced copy of a picture instead of the picture itself, for
   *        pictures whose own planes are not readable.
   *
   * The rectangle such a frame detects is in the reduction's coordinates; scaling it back to
   * coded space is the caller's job.
   *
   * \param picture the picture the reduction was taken from, for its stereo mode
   * \param hint the stream, for the same color range decision the full-size path makes
   */
  static bool BuildGeometryFrameRef(const KODI::VIDEO::GEOMETRY::ReducedFrame& reduction,
                                    const VideoPicture& picture,
                                    const CDVDStreamInfo& hint,
                                    KODI::VIDEO::GEOMETRY::FrameRef& frame);
};
