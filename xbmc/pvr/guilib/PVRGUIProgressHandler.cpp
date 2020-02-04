/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIProgressHandler.h"

#include "ServiceBroker.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace PVR
{
  CPVRGUIProgressHandler::CPVRGUIProgressHandler(const std::string& strTitle)
  : CThread("PVRGUIProgressHandler"),
    m_strTitle(strTitle),
    m_fProgress(0.0f),
    m_bChanged(false)
  {
    Create(true /* bAutoDelete */);
  }

  void CPVRGUIProgressHandler::UpdateProgress(const std::string& strText, float fProgress)
  {
    CSingleLock lock(m_critSection);
    m_bChanged = true;
    m_strText = strText;
    m_fProgress = fProgress;
  }

  void CPVRGUIProgressHandler::UpdateProgress(const std::string& strText, int iCurrent, int iMax)
  {
    float fPercentage = (iCurrent * 100.0f) / iMax;
    if (!std::isnan(fPercentage))
      fPercentage = std::min(100.0f, fPercentage);

    UpdateProgress(strText, fPercentage);
  }

  void CPVRGUIProgressHandler::DestroyProgress()
  {
    CSingleLock lock(m_critSection);
    m_bStop = true;
    m_bChanged = false;
  }

  void CPVRGUIProgressHandler::Process()
  {
    CGUIDialogExtendedProgressBar* progressBar = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogExtendedProgressBar>(WINDOW_DIALOG_EXT_PROGRESS);
    if (m_bStop || !progressBar)
      return;

    CGUIDialogProgressBarHandle* progressHandle = progressBar->GetHandle(m_strTitle);
    if (!progressHandle)
      return;

    while (!m_bStop)
    {
      float fProgress = 0.0;
      std::string strText;
      bool bUpdate = false;

      {
        CSingleLock lock(m_critSection);
        if (m_bChanged)
        {
          m_bChanged = false;
          fProgress = m_fProgress;
          strText = m_strText;
          bUpdate = true;
        }
      }

      if (bUpdate)
      {
        progressHandle->SetPercentage(fProgress);
        progressHandle->SetText(strText);
      }

      CThread::Sleep(
          100); // Intentionally ignore some changes that come in too fast. Humans cannot read as fast as Mr. Data ;-)
    }

    progressHandle->MarkFinished();
  }

} // namespace PVR
