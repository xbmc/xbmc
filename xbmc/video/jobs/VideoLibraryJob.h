/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Job.h"

class CVideoDatabase;

/*!
 \brief Basic implementation/interface of a CJob which interacts with the
 video database.
 */
class CVideoLibraryJob : public CJob
{
public:
  ~CVideoLibraryJob() override;

  /*!
   \brief Whether the job can be cancelled or not.
   */
  virtual bool CanBeCancelled() const { return false; }

  /*!
   \brief Tries to cancel the running job.

   \return True if the job was cancelled, false otherwise
  */
  virtual bool Cancel() { return false; }

  // implementation of CJob
  bool DoWork() override;
  const char *GetType() const override { return "VideoLibraryJob"; }
  bool operator==(const CJob* job) const override { return false; }

protected:
  CVideoLibraryJob();

  /*!
   \brief Worker method to be implemented by an actual implementation.

   \param[in] db Already open video database to be used for interaction
   \return True if the process succeeded, false otherwise
   */
  virtual bool Work(CVideoDatabase &db) = 0;
};
