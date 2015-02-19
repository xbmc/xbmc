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

#include "ProgressJob.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"

using namespace std;

CProgressJob::CProgressJob(CGUIDialogProgressBarHandle* progressBar)
  : m_progress(progressBar)
{ }

CProgressJob::~CProgressJob()
{
  MarkFinished();

  m_progress = NULL;
}

void CProgressJob::SetTitle(const std::string &title)
{
  if (m_progress == NULL)
    return;

  m_progress->SetTitle(title);
}

void CProgressJob::SetText(const std::string &text)
{
  if (m_progress == NULL)
    return;

  m_progress->SetText(text);
}

void CProgressJob::SetProgress(float percentage)
{
  if (m_progress == NULL)
    return;

  m_progress->SetPercentage(percentage);
}

void CProgressJob::SetProgress(int currentStep, int totalSteps)
{
  if (m_progress == NULL)
    return;

  m_progress->SetProgress(currentStep, totalSteps);
}

void CProgressJob::MarkFinished()
{
  if (m_progress == NULL)
    return;

  m_progress->MarkFinished();
}
