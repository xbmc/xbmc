#pragma once
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

#include <string>

#include "FileItem.h"
#include "video/jobs/VideoLibraryProgressJob.h"

/*!
 \brief Video library job implementation for refreshing a single item.
*/
class CVideoLibraryRefreshingJob : public CVideoLibraryProgressJob
{
public:
  /*!
   \brief Creates a new video library cleaning job for the given paths.

   \param[inout] item Video item to be refreshed
   \param[in] forceRefresh Whether to force a complete refresh (including NFO or internet lookup)
   \param[in] refreshAll Whether to refresh all sub-items (in case of a tvshow)
   \param[in] ignoreNfo Whether or not to ignore local NFO files
   \param[in] searchTitle Title to use for the search (instead of determining it from the item's filename/path)
  */
  CVideoLibraryRefreshingJob(CFileItemPtr item, bool forceRefresh, bool refreshAll, bool ignoreNfo = false, const std::string& searchTitle = "");

  virtual ~CVideoLibraryRefreshingJob();

  // specialization of CJob
  virtual const char *GetType() const { return "VideoLibraryRefreshingJob"; }
  virtual bool operator==(const CJob* job) const;

protected:
  // implementation of CVideoLibraryJob
  virtual bool Work(CVideoDatabase &db);

private:
  CFileItemPtr m_item;
  bool m_forceRefresh;
  bool m_refreshAll;
  bool m_ignoreNfo;
  std::string m_searchTitle;
};
