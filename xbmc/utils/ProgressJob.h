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

class CProgressJob : public CJob
{
public:
  virtual ~CProgressJob();

  // implementation of CJob
  virtual const char *GetType() const { return "ProgressJob"; }
  virtual bool operator==(const CJob* job) const { return false; }

protected:
  CProgressJob(CGUIDialogProgressBarHandle* progressBar);

  CGUIDialogProgressBarHandle* GetProgressBar() { return m_progress; }

  void SetTitle(const std::string &title);
  void SetText(const std::string &text);

  void SetProgress(float percentage);
  void SetProgress(int currentStep, int totalSteps);
  void MarkFinished();

private:
  CGUIDialogProgressBarHandle* m_progress;
};
