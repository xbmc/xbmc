#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include <vector>

#include "utils/StdString.h"

class CBookmark
{
public:
  CBookmark();
  void Reset();

  /*! \brief returns true if this bookmark has been set.
   \return true if totalTimeInSeconds is positive.
   */
  bool IsSet() const;

  /*! \brief returns true if this bookmark is part way through the video file
   \return true if both totalTimeInSeconds and timeInSeconds are positive.
   */
  bool IsPartWay() const;

  double timeInSeconds;
  double totalTimeInSeconds;
  long partNumber;
  CStdString thumbNailImage;
  CStdString playerState;
  CStdString player;
  long seasonNumber;
  long episodeNumber;

  enum EType
  {
    STANDARD = 0,
    RESUME = 1,
    EPISODE = 2
  } type;
};

typedef std::vector<CBookmark> VECBOOKMARKS;

