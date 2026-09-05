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

//! \brief Decodes a file at several points and describes each picture to the detector.
class CDVDFileGeometry
{
public:
  //! \brief Sample a file at several points and work out its picture rectangle. Decodes in
  //! software with film grain synthesis off. \p cancelled is asked between positions, and a
  //! scan reporting itself cancelled must not be stored.
  static KODI::VIDEO::GEOMETRY::SampledGeometry ExtractContentGeometry(
      const CFileItem& fileItem,
      const KODI::VIDEO::GEOMETRY::SamplingParams& sampling = {},
      const KODI::VIDEO::GEOMETRY::CombinerParams& combining = {},
      const std::function<bool()>& cancelled = {});

  //! \brief Describe a decoded picture to the detector. Reads the layout from the pixel format
  //! descriptor, so an unfamiliar one is described correctly or refused. Fails for a picture
  //! whose buffer exposes no planes.
  static bool BuildGeometryFrameRef(const VideoPicture& picture,
                                    const CDVDStreamInfo& hint,
                                    KODI::VIDEO::GEOMETRY::FrameRef& frame);

  //! \brief Point a FrameRef at a reduced copy instead of the picture itself. What such a
  //! frame detects is in the reduction's coordinates, and scaling it back is the caller's job.
  static bool BuildGeometryFrameRef(const KODI::VIDEO::GEOMETRY::ReducedFrame& reduction,
                                    const VideoPicture& picture,
                                    const CDVDStreamInfo& hint,
                                    KODI::VIDEO::GEOMETRY::FrameRef& frame);
};
