/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

#include "utils/Job.h"

class CGUIDialogProgress;
class CGUIDialogProgressBarHandle;

/*!
  \brief Basic implementation of a CJob with a progress bar to indicate the
  progress of the job being processed.
 */
class CProgressJob : public CJob
{
public:
  ~CProgressJob() override;

  // implementation of CJob
  const char *GetType() const override { return "ProgressJob"; }
  bool operator==(const CJob* job) const override { return false; }
  bool ShouldCancel(unsigned int progress, unsigned int total) const override;

  /*!
   \brief Executes the job showing a modal progress dialog.
   */
  bool DoModal();

  /*!
   \brief Sets the given progress indicators to be used during execution of
   the job.

   \details This automatically disables auto-closing the given progress
   indicators once the job has been finished.

   \param progressBar Progress bar handle to be used.
   \param progressDialog Progress dialog to be used.
   \param updateProgress (optional) Whether to show progress updates.
   \param updateInformation (optional) Whether to show progress information.
   */
  void SetProgressIndicators(CGUIDialogProgressBarHandle* progressBar, CGUIDialogProgress* progressDialog, bool updateProgress = true, bool updateInformation = true);

  bool HasProgressIndicator() const;

protected:
  CProgressJob();
  explicit CProgressJob(CGUIDialogProgressBarHandle* progressBar);

  /*!
   \brief Whether the job is being run modally or in the background.
   */
  bool IsModal() const { return m_modal; }

  /*!
   \brief Returns the progress bar indicating the progress of the job.
   */
  CGUIDialogProgressBarHandle* GetProgressBar() const { return m_progress; }

  /*!
   \brief Sets the progress bar indicating the progress of the job.
   */
  void SetProgressBar(CGUIDialogProgressBarHandle* progress) { m_progress = progress; }

  /*!
   \brief Returns the progress dialog indicating the progress of the job.
   */
  CGUIDialogProgress* GetProgressDialog() const { return m_progressDialog; }

  /*!
   \brief Sets the progress bar indicating the progress of the job.
   */
  void SetProgressDialog(CGUIDialogProgress* progressDialog) { m_progressDialog = progressDialog; }

  /*!
   \brief Whether to automatically close the progress indicator in MarkFinished().
   */
  bool GetAutoClose() { return m_autoClose; }

  /*!
   \brief Set whether to automatically close the progress indicator in MarkFinished().
   */
  void SetAutoClose(bool autoClose) { m_autoClose = autoClose; }

  /*!
   \brief Whether to update the progress bar or not.
   */
  bool GetUpdateProgress() { return m_updateProgress; }

  /*!
   \brief Set whether to update the progress bar or not.
   */
  void SetUpdateProgress(bool updateProgress) { m_updateProgress = updateProgress; }

  /*!
  \brief Whether to update the progress information or not.
  */
  bool GetUpdateInformation() { return m_updateInformation; }

  /*!
  \brief Set whether to update the progress information or not.
  */
  void SetUpdateInformation(bool updateInformation) { m_updateInformation = updateInformation; }

  /*!
   \brief Makes sure that the modal dialog is being shown.
   */
  void ShowProgressDialog() const;

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
  void SetProgress(float percentage) const;

  /*!
   \brief Sets the progress of the progress bar to the given value.

   \param[in] currentStep Current step being processed
   \param[in] totalSteps Total steps to be processed
  */
  void SetProgress(int currentStep, int totalSteps) const;

  /*!
   \brief Marks the progress as finished by setting it to 100%.
   */
  void MarkFinished();

  /*!
   \brief Checks if the progress dialog has been cancelled.
   */
  bool IsCancelled() const;

private:
  bool m_modal = false;
  bool m_autoClose = true;
  bool m_updateProgress = true;
  bool m_updateInformation = true;
  mutable CGUIDialogProgressBarHandle* m_progress;
  mutable CGUIDialogProgress* m_progressDialog;
};
