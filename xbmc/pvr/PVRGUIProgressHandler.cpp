/*
 *      Copyright (C) 2017 Team Kodi
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

#include "PVRGUIProgressHandler.h"

#include <algorithm>
#include <cmath>

#include "ServiceBroker.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"

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

  void CPVRGUIProgressHandler::UpdateProgress(const std::string &strText, float fProgress)
  {
    CSingleLock lock(m_critSection);
    m_bChanged = true;
    m_strText = strText;
    m_fProgress = fProgress;
  }

  void CPVRGUIProgressHandler::UpdateProgress(const std::string &strText, int iCurrent, int iMax)
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

      Sleep(100); // Intentionally ignore some changes that come in too fast. Humans cannot read as fast as Mr. Data ;-)
    }

    progressHandle->MarkFinished();
  }

} // namespace PVR
