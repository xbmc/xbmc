#pragma once
/*
 *      Copyright (C) 2015 Team XBMC
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

#include "utils/Job.h"

class CGUIDialogProgressBarHandle;

/*!
  \brief Basic implementation of a CJob with a progress bar to indicate the
  progress of the job being processed.
 */
class CProgressJob : public CJob
{
public:
  virtual ~CProgressJob();

  // implementation of CJob
  virtual const char *GetType() const { return "ProgressJob"; }
  virtual bool operator==(const CJob* job) const { return false; }

protected:
  CProgressJob(CGUIDialogProgressBarHandle* progressBar);

  /*!
   \brief Returns the progress bar indicating the progress of the job.
   */
  CGUIDialogProgressBarHandle* GetProgressBar() { return m_progress; }

  /*!
   \brief Sets the given title as the title of the progress bar.

   \param[in] title Title to be set
   */
  void SetTitle(const std::string &title);

  /*!
   \brief Sets the given text as the description of the progress bar.

   \param[in] text Text to be set
  */
  void SetText(const std::string &text);

  /*!
   \brief Sets the progress of the progress bar to the given value in percentage.

   \param[in] percentage Percentage to be set as the current progress
  */
  void SetProgress(float percentage);

  /*!
   \brief Sets the progress of the progress bar to the given value.

   \param[in] currentStep Current step being processed
   \param[in] totalSteps Total steps to be processed
  */
  void SetProgress(int currentStep, int totalSteps);

  /*!
   \brief Marks the progress as finished by setting it to 100%.
   */
  void MarkFinished();

private:
  CGUIDialogProgressBarHandle* m_progress;
};
