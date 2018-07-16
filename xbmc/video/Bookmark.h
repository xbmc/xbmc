/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

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
  std::string thumbNailImage;
  std::string playerState;
  std::string player;
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

