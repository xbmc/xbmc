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
#include <mutex>
#include <string>

using namespace std::chrono_literals;

namespace PVR
{

CPVRGUIProgressHandler::CPVRGUIProgressHandler(const std::string& strTitle)
  : CThread("PVRGUIProgressHandler"), m_strTitle(strTitle)
{
}

void CPVRGUIProgressHandler::UpdateProgress(const std::string& strText, float fProgress)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_bChanged = true;
  m_strText = strText;
  m_fProgress = fProgress;

  if (!m_bCreated)
  {
    m_bCreated = true;
    Create();
  }
}

void CPVRGUIProgressHandler::UpdateProgress(const std::string& strText, int iCurrent, int iMax)
{
  float fPercentage = (iCurrent * 100.0f) / iMax;
  if (!std::isnan(fPercentage))
    fPercentage = std::min(100.0f, fPercentage);

  UpdateProgress(strText, fPercentage);
}

void CPVRGUIProgressHandler::Process()
{
  CGUIDialogExtendedProgressBar* progressBar =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogExtendedProgressBar>(
          WINDOW_DIALOG_EXT_PROGRESS);
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
      std::unique_lock<CCriticalSection> lock(m_critSection);
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

    // Intentionally ignore some changes that come in too fast. Humans cannot read as fast as Mr. Data ;-)
    CThread::Sleep(100ms);
  }

  progressHandle->MarkFinished();
}

} // namespace PVR
