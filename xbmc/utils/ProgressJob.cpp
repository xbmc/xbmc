/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProgressJob.h"

#include "ServiceBroker.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/Variant.h"

#include <math.h>

CProgressJob::CProgressJob()
  : m_progress(NULL),
    m_progressDialog(NULL)
{ }

CProgressJob::CProgressJob(CGUIDialogProgressBarHandle* progressBar)
  : m_progress(progressBar),
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
    m_progressDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);

    if (m_progressDialog == NULL)
      return false;
  }

  m_modal = true;

  // do the work
  bool result = DoWork();

  // mark the progress dialog as finished (will close it)
  MarkFinished();
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
  m_progressDialog->Open();
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
    m_progressDialog->SetHeading(CVariant{title});

    // Prevent displaying the progress dialog without any heading and/or text.
    if (m_progressDialog->HasHeading() && m_progressDialog->HasText())
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
    m_progressDialog->SetText(CVariant{text});

    // Prevent displaying the progress dialog without any heading and/or text.
    if (m_progressDialog->HasText() && m_progressDialog->HasHeading())
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
    {
      m_progress->MarkFinished();
      // We don't own this pointer and it will be deleted after it's marked finished
      // just set it to nullptr so we don't try to use it again
      m_progress = nullptr;
    }
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
