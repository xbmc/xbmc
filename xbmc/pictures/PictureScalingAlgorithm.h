/*
 *      Copyright (C) 2015 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
