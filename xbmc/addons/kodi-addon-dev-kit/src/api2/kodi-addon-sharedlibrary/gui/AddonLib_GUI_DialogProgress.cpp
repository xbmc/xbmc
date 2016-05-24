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
#include KITINCLUDE(ADDON_API_LEVEL, gui/DialogProgress.hpp)

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  CDialogProgress::CDialogProgress()
  {
    m_DialogHandle = g_interProcess.m_Callbacks->GUI.Dialogs.Progress.New(g_interProcess.m_Handle);
    if (!m_DialogHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CDialogProgress can't create window class from Kodi !!!\n");
  }

  CDialogProgress::~CDialogProgress()
  {
    if (m_DialogHandle)
      g_interProcess.m_Callbacks->GUI.Dialogs.Progress.Delete(g_interProcess.m_Handle, m_DialogHandle);
  }

  void CDialogProgress::Open()
  {
    g_interProcess.m_Callbacks->GUI.Dialogs.Progress.Open(g_interProcess.m_Handle, m_DialogHandle);
  }

  void CDialogProgress::SetHeading(const std::string& title)
  {
    g_interProcess.m_Callbacks->GUI.Dialogs.Progress.SetHeading(g_interProcess.m_Handle, m_DialogHandle, title.c_str());
  }

  void CDialogProgress::SetLine(unsigned int iLine, const std::string& line)
  {
    g_interProcess.m_Callbacks->GUI.Dialogs.Progress.SetLine(g_interProcess.m_Handle, m_DialogHandle, iLine, line.c_str());
  }

  void CDialogProgress::SetCanCancel(bool bCanCancel)
  {
    g_interProcess.m_Callbacks->GUI.Dialogs.Progress.SetCanCancel(g_interProcess.m_Handle, m_DialogHandle, bCanCancel);
  }

  bool CDialogProgress::IsCanceled() const
  {
    return g_interProcess.m_Callbacks->GUI.Dialogs.Progress.IsCanceled(g_interProcess.m_Handle, m_DialogHandle);
  }

  void CDialogProgress::SetPercentage(int iPercentage)
  {
    g_interProcess.m_Callbacks->GUI.Dialogs.Progress.SetPercentage(g_interProcess.m_Handle, m_DialogHandle, iPercentage);
  }

  int CDialogProgress::GetPercentage() const
  {
    return g_interProcess.m_Callbacks->GUI.Dialogs.Progress.GetPercentage(g_interProcess.m_Handle, m_DialogHandle);
  }

  void CDialogProgress::ShowProgressBar(bool bOnOff)
  {
    g_interProcess.m_Callbacks->GUI.Dialogs.Progress.ShowProgressBar(g_interProcess.m_Handle, m_DialogHandle, bOnOff);
  }

  void CDialogProgress::SetProgressMax(int iMax)
  {
    g_interProcess.m_Callbacks->GUI.Dialogs.Progress.SetProgressMax(g_interProcess.m_Handle, m_DialogHandle, iMax);
  }

  void CDialogProgress::SetProgressAdvance(int nSteps)
  {
    g_interProcess.m_Callbacks->GUI.Dialogs.Progress.SetProgressAdvance(g_interProcess.m_Handle, m_DialogHandle, nSteps);
  }

  bool CDialogProgress::Abort()
  {
    return g_interProcess.m_Callbacks->GUI.Dialogs.Progress.Abort(g_interProcess.m_Handle, m_DialogHandle);
  }

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
