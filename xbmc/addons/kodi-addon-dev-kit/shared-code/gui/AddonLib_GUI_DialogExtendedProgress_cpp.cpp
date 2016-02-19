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
#include "kodi/api2/gui/DialogExtendedProgress.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{

  CDialogExtendedProgress::CDialogExtendedProgress(const std::string& title)
  {
    m_DialogHandle = g_interProcess.Dialogs_ExtendedProgress_New(title);
    if (!m_DialogHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CDialogExtendedProgress can't create window class from Kodi !!!\n");
  }

  CDialogExtendedProgress::~CDialogExtendedProgress()
  {
    if (m_DialogHandle)
      g_interProcess.Dialogs_ExtendedProgress_Delete(m_DialogHandle);
  }

  std::string CDialogExtendedProgress::Title() const
  {
    return g_interProcess.Dialogs_ExtendedProgress_Title(m_DialogHandle);
  }

  void CDialogExtendedProgress::SetTitle(const std::string& title)
  {
    g_interProcess.Dialogs_ExtendedProgress_SetTitle(m_DialogHandle, title);
  }

  std::string CDialogExtendedProgress::Text() const
  {
    return g_interProcess.Dialogs_ExtendedProgress_Text(m_DialogHandle);
  }
    
  void CDialogExtendedProgress::SetText(const std::string& text)
  {
    g_interProcess.Dialogs_ExtendedProgress_SetText(m_DialogHandle, text);
  }
    
  bool CDialogExtendedProgress::IsFinished() const
  {
    return g_interProcess.Dialogs_ExtendedProgress_IsFinished(m_DialogHandle);
  }
    
  void CDialogExtendedProgress::MarkFinished()
  {
    g_interProcess.Dialogs_ExtendedProgress_MarkFinished(m_DialogHandle);
  }
    
  float CDialogExtendedProgress::Percentage() const
  {
    return g_interProcess.Dialogs_ExtendedProgress_Percentage(m_DialogHandle);
  }
    
  void CDialogExtendedProgress::SetPercentage(float fPercentage)
  {
    g_interProcess.Dialogs_ExtendedProgress_SetPercentage(m_DialogHandle, fPercentage);
  }
    
  void CDialogExtendedProgress::SetProgress(int currentItem, int itemCount)
  {
    g_interProcess.Dialogs_ExtendedProgress_SetProgress(m_DialogHandle, currentItem, itemCount);
  }

}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
