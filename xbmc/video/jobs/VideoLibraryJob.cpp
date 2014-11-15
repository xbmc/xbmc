/*
 *      Copyright (C) 2014 Team XBMC
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

#include "VideoLibraryJob.h"
#include "video/VideoDatabase.h"

using namespace std;

CVideoLibraryJob::CVideoLibraryJob()
{ }

CVideoLibraryJob::~CVideoLibraryJob()
{ }

bool CVideoLibraryJob::DoWork()
{
  CVideoDatabase db;
  if (!db.Open())
    return false;

  return Work(db);
}
