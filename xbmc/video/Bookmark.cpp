/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Bookmark.h"

CBookmark::CBookmark()
{
  Reset();
}

void CBookmark::Reset()
{
  episodeNumber = 0;
  seasonNumber = 0;
  timeInSeconds = 0.0;
  totalTimeInSeconds = 0.0;
  partNumber = 0;
  type = STANDARD;
}

bool CBookmark::IsSet() const
{
  return totalTimeInSeconds > 0.0;
}

bool CBookmark::IsPartWay() const
{
  return totalTimeInSeconds > 0.0 && timeInSeconds > 0.0;
}
