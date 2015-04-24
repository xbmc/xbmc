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

#include <math.h>

#include "ProgressJob.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "guilib/GUIWindowManager.h"

using namespace std;

CProgressJob::CProgressJob()
  : m_modal(false),
    m_autoClose(true),
    m_updateProgress(true),
    m_updateInformation(true),
    m_progress(NULL),
    m_progressDialog(NULL)
{ }

CProgressJob::CProgressJob(CGUIDialogProgressBarHandle* progressBar)
  : m_modal(false),
    m_autoClose(true),
    m_updateProgress(true),
    m_updateInformation(true),
    m_progress(progressBar),
    m_progressDialog(NULL)
{ }

CProgressJob::~CProgressJob()
{
  MarkFinished();

  m_progress = NULL;
  m_progressDialog = NULL;
}

bool CProgressJob::ShouldCancel(unsigned int progress, unsigned int total) const
{
  if (IsCancelled())
    return true;

  SetProgress(progress, total);

  return CJob::ShouldCancel(progress, total);
}

bool CProgressJob::DoModal()
{
  m_progress = NULL;

  // get a progress dialog if we don't already have one
  if (m_progressDialog == NULL)
  {
    m_progressDialog = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

    if (m_progressDialog == NULL)
      return false;
  }

  m_modal = true;

  // do the work
  bool result = DoWork();

  // close the progress dialog
  if (m_autoClose)
    m_progressDialog->Close();
  m_modal = false;

  return result;
}

void CProgressJob::SetProgressIndicators(CGUIDialogProgressBarHandle* progressBar, CGUIDialogProgress* progressDialog, bool updateProgress /* = true */, bool updateInformation /* = true */)
{
  SetProgressBar(progressBar);
  SetProgressDialog(progressDialog);
  SetUpdateProgress(updateProgress);
  SetUpdateInformation(updateInformation);

  // disable auto-closing
  SetAutoClose(false);
}

void CProgressJob::ShowProgressDialog() const
{
  if (!IsModal() || m_progressDialog == NULL ||
      m_progressDialog->IsDialogRunning())
    return;

  // show the progress dialog as a modal dialog with a progress bar
  m_progressDialog->StartModal();
  m_progressDialog->ShowProgressBar(true);
}

void CProgressJob::SetTitle(const std::string &title)
{
  if (!m_updateInformation)
    return;

  if (m_progress != NULL)
    m_progress->SetTitle(title);
  else if (m_progressDialog != NULL)
  {
    m_progressDialog->SetHeading(title);

    ShowProgressDialog();
  }
}

void CProgressJob::SetText(const std::string &text)
{
  if (!m_updateInformation)
    return;

  if (m_progress != NULL)
    m_progress->SetText(text);
  else if (m_progressDialog != NULL)
  {
    m_progressDialog->SetText(text);

    ShowProgressDialog();
  }
}

void CProgressJob::SetProgress(float percentage) const
{
  if (!m_updateProgress)
    return;

  if (m_progress != NULL)
    m_progress->SetPercentage(percentage);
  else if (m_progressDialog != NULL)
  {
    ShowProgressDialog();

    int iPercentage = static_cast<int>(ceil(percentage));
    // only change and update the progress bar if its percentage value changed
    // (this can have a huge impact on performance if it's called a lot)
    if (iPercentage != m_progressDialog->GetPercentage())
    {
      m_progressDialog->SetPercentage(iPercentage);
      m_progressDialog->Progress();
    }
  }
}

void CProgressJob::SetProgress(int currentStep, int totalSteps) const
{
  if (!m_updateProgress)
    return;

  if (m_progress != NULL)
    m_progress->SetProgress(currentStep, totalSteps);
  else if (m_progressDialog != NULL)
    SetProgress((static_cast<float>(currentStep) * 100.0f) / totalSteps);
}

void CProgressJob::MarkFinished()
{
  if (m_progress != NULL)
  {
    if (m_updateProgress)
      m_progress->MarkFinished();
  }
  else if (m_progressDialog != NULL && m_autoClose)
    m_progressDialog->Close();
}

bool CProgressJob::IsCancelled() const
{
  if (m_progressDialog != NULL)
    return m_progressDialog->IsCanceled();

  return false;
}

bool CProgressJob::HasProgressIndicator() const
{
  return m_progress != nullptr || m_progressDialog != nullptr;
}
