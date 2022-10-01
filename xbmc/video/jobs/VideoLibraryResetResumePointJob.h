/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "video/jobs/VideoLibraryJob.h"

#include <memory>

/*!
 \brief Video library job implementation for resetting a resume point.
 */
class CVideoLibraryResetResumePointJob : public CVideoLibraryJob
{
public:
  /*!
   \brief Creates a new job for resetting a given item's resume point.

   \param[in] item Item for that the resume point shall be reset.
  */
  CVideoLibraryResetResumePointJob(const std::shared_ptr<CFileItem>& item);
  ~CVideoLibraryResetResumePointJob() override = default;

  const char *GetType() const override { return "CVideoLibraryResetResumePointJob"; }
  bool operator==(const CJob* job) const override;

protected:
  bool Work(CVideoDatabase &db) override;

private:
  std::shared_ptr<CFileItem> m_item;
};
