/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
