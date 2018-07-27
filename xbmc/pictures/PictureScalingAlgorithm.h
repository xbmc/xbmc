/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <map>

class CPictureScalingAlgorithm
{
public:
  typedef enum Algorithm
  {
    NoAlgorithm,
    FastBilinear,
    Bilinear,
    Bicubic,
    Experimental,
    NearestNeighbor,
    AveragingArea,
    Bicublin,
    Gaussian,
    Sinc,
    Lanczos,
    BicubicSpline
  } Algorithm;

  static Algorithm Default;

  static Algorithm FromString(const std::string& scalingAlgorithm);
  static std::string ToString(Algorithm scalingAlgorithm);
  static int ToSwscale(const std::string& scalingAlgorithm);
  static int ToSwscale(Algorithm scalingAlgorithm);

private:
  CPictureScalingAlgorithm();

  typedef struct ScalingAlgorithm
  {
    std::string name;
    int swscale;
  } ScalingAlgorithm;

  typedef std::map<CPictureScalingAlgorithm::Algorithm, CPictureScalingAlgorithm::ScalingAlgorithm> AlgorithmMap;
  static AlgorithmMap m_algorithms;
};
