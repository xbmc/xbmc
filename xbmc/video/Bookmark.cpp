/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
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
  timeInSeconds = 0.0f;
  totalTimeInSeconds = 0.0f;
  partNumber = 0;
  type = STANDARD;
}

bool CBookmark::IsSet() const
{
  return totalTimeInSeconds > 0.0f;
}

bool CBookmark::IsPartWay() const
{
  return totalTimeInSeconds > 0.0f && timeInSeconds > 0.0f;
}
