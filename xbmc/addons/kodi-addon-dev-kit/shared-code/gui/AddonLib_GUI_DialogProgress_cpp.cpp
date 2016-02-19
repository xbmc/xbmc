/*
 *      Copyright (C) 2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "InterProcess.h"
#include "kodi/api2/gui/DialogProgress.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{

  CDialogProgress::CDialogProgress()
  {
    m_DialogHandle = g_interProcess.Dialogs_Progress_New();
    if (!m_DialogHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CDialogProgress can't create window class from Kodi !!!\n"); \
  }

  CDialogProgress::~CDialogProgress()
  {
    if (m_DialogHandle)
      g_interProcess.Dialogs_Progress_Delete(m_DialogHandle);
  }

  void CDialogProgress::Open()
  {
    g_interProcess.Dialogs_Progress_Open(m_DialogHandle);
  }

  void CDialogProgress::SetHeading(const std::string& title)
  {
    g_interProcess.Dialogs_Progress_SetHeading(m_DialogHandle, title);
  }

  void CDialogProgress::SetLine(unsigned int iLine, const std::string& line)
  {
    g_interProcess.Dialogs_Progress_SetLine(m_DialogHandle, iLine, line);
  }

  void CDialogProgress::SetCanCancel(bool bCanCancel)
  {
    g_interProcess.Dialogs_Progress_SetCanCancel(m_DialogHandle, bCanCancel);
  }

  bool CDialogProgress::IsCanceled() const
  {
    return g_interProcess.Dialogs_Progress_IsCanceled(m_DialogHandle);
  }

  void CDialogProgress::SetPercentage(int iPercentage)
  {
    g_interProcess.Dialogs_Progress_SetPercentage(m_DialogHandle, iPercentage);
  }

  int CDialogProgress::GetPercentage() const
  {
    return g_interProcess.Dialogs_Progress_GetPercentage(m_DialogHandle);
  }

  void CDialogProgress::ShowProgressBar(bool bOnOff)
  {
    g_interProcess.Dialogs_Progress_ShowProgressBar(m_DialogHandle, bOnOff);
  }

  void CDialogProgress::SetProgressMax(int iMax)
  {
    g_interProcess.Dialogs_Progress_SetProgressMax(m_DialogHandle, iMax);
  }

  void CDialogProgress::SetProgressAdvance(int nSteps)
  {
    g_interProcess.Dialogs_Progress_SetProgressAdvance(m_DialogHandle, nSteps);
  }

  bool CDialogProgress::Abort()
  {
    return g_interProcess.Dialogs_Progress_Abort(m_DialogHandle);
  }

}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
