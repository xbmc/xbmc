/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <algorithm>

extern "C" {
#include <libswscale/swscale.h>
}

#include "PictureScalingAlgorithm.h"
#include "utils/StringUtils.h"

CPictureScalingAlgorithm::Algorithm CPictureScalingAlgorithm::Default = CPictureScalingAlgorithm::Bicubic;

CPictureScalingAlgorithm::AlgorithmMap CPictureScalingAlgorithm::m_algorithms = {
  { FastBilinear,     { "fast_bilinear",    SWS_FAST_BILINEAR } },
  { Bilinear,         { "bilinear",         SWS_BILINEAR } },
  { Bicubic,          { "bicubic",          SWS_BICUBIC } },
  { Experimental,     { "experimental",     SWS_X } },
  { NearestNeighbor,  { "nearest_neighbor", SWS_POINT } },
  { AveragingArea,    { "averaging_area",   SWS_AREA } },
  { Bicublin,         { "bicublin",         SWS_BICUBLIN } },
  { Gaussian,         { "gaussian",         SWS_GAUSS } },
  { Sinc,             { "sinc",             SWS_SINC } },
  { Lanczos,          { "lanczos",          SWS_LANCZOS } },
  { BicubicSpline,    { "bicubic_spline",   SWS_SPLINE } },
};

CPictureScalingAlgorithm::Algorithm CPictureScalingAlgorithm::FromString(const std::string& scalingAlgorithm)
{
  const auto& algorithm = std::find_if(m_algorithms.begin(), m_algorithms.end(),
    [&scalingAlgorithm](const std::pair<Algorithm, ScalingAlgorithm>& algo) { return StringUtils::EqualsNoCase(algo.second.name, scalingAlgorithm); });
  if (algorithm != m_algorithms.end())
    return algorithm->first;

  return NoAlgorithm;
}

std::string CPictureScalingAlgorithm::ToString(Algorithm scalingAlgorithm)
{
  const auto& algorithm = m_algorithms.find(scalingAlgorithm);
  if (algorithm != m_algorithms.end())
    return algorithm->second.name;

  return "";
}

int CPictureScalingAlgorithm::ToSwscale(const std::string& scalingAlgorithm)
{
  return ToSwscale(FromString(scalingAlgorithm));
}

int CPictureScalingAlgorithm::ToSwscale(Algorithm scalingAlgorithm)
{
  const auto& algorithm = m_algorithms.find(scalingAlgorithm);
  if (algorithm != m_algorithms.end())
    return algorithm->second.swscale;

  return ToSwscale(Default);
}
