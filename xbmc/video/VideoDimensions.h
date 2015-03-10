#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <string>
#include <vector>

/*
 * Represents the boundaries for a specific resolution, i.e. the limits that
 * each dimension has
 */
struct SResolutionBoundaries
{
  struct SLimits
  {
    int min;
    int max;
  };

  SLimits width;
  SLimits height;
};

/*
 * Represents a resolution, identified mainly by a quality, e.g. "720"
 */
struct SResolution
{
  std::string quality;
  std::string fidelity;
  SResolutionBoundaries boundaries;
};

class CVideoDimensions
{
public:
  CVideoDimensions(int width, int height);
  ~CVideoDimensions() {};

  /*
   * @return the quality for the resolution, e.g. "720"
   */
  std::string GetQuality() const;

  /*
   * @return the fidelity of the resolution, e.g. "SD" or "HD"
   */
  std::string GetFidelity() const;

  /*
   * @return the dimensional boundaries for this resolution
   */
  SResolutionBoundaries GetBoundaries() const;

  /*
   * @return the dimensional boundaries for the specified resolution quality
   */
  static SResolutionBoundaries GetQualityBoundaries(const std::string &quality);

  /*
   * @return the dimensional boundaries for the specified fidelity
   */
  static SResolutionBoundaries GetFidelityBoundaries(const std::string &fidelity);

  /*
   * @return a list of all possible qualities
   */
  static const std::vector<std::string> GetQualities();

  /*
   * @return a list of all possible fidelities
   */
  static const std::vector<std::string> GetFidelities();

private:
  int m_width;
  int m_height;
  SResolution m_resolution;

  /*
   * Resolution data for mapping a width and a height to a specific resolution
   */
  static std::vector<SResolution> resolutions;
};
