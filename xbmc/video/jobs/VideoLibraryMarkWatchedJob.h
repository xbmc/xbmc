/*
 *      Copyright (C) 2014 Team XBMC
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

#include "FileItem.h"
#include "video/jobs/VideoLibraryJob.h"

/*!
 \brief Video library job implementation for marking items as watched/unwatched.
 */
class CVideoLibraryMarkWatchedJob : public CVideoLibraryJob
{
public:
  /*!
   \brief Creates a new video library scanning job.

   \param[in] item Item to be marked as watched/unwatched
   \param[in] mark Whether to mark the item as watched or unwatched
  */
  CVideoLibraryMarkWatchedJob(const CFileItemPtr &item, bool mark);
  ~CVideoLibraryMarkWatchedJob() override;

  const char *GetType() const override { return "CVideoLibraryMarkWatchedJob"; }
  bool operator==(const CJob* job) const override;

protected:
  bool Work(CVideoDatabase &db) override;

private:
  CFileItemPtr m_item;
  bool m_mark;
};
