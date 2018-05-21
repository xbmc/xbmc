#pragma once
/*
 *      Copyright (C) 2017 Team XBMC
 *      http://kodi.tv
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

#include <set>

#include "music/jobs/MusicLibraryProgressJob.h"

/*!
 \brief Music library job implementation for cleaning the video library.
*/
class CMusicLibraryCleaningJob : public CMusicLibraryProgressJob
{
public:
  /*!
   \brief Creates a new music library cleaning job.
   \param[in] progressDialog Progress dialog to be used to display the cleaning progress
  */
  CMusicLibraryCleaningJob(CGUIDialogProgress* progressDialog);
  ~CMusicLibraryCleaningJob() override;

  // specialization of CJob
  const char *GetType() const override { return "MusicLibraryCleaningJob"; }
  bool operator==(const CJob* job) const override;

protected:
  // implementation of CMusicLibraryJob
  bool Work(CMusicDatabase &db) override;

private:
 
};
