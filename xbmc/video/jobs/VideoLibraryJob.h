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
