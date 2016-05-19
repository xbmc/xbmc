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
#include KITINCLUDE(ADDON_API_LEVEL, gui/DialogExtendedProgress.hpp)

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  CDialogExtendedProgress::CDialogExtendedProgress(const std::string& title)
  {
    m_DialogHandle = g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.New(g_interProcess.m_Handle, title.c_str());
    if (!m_DialogHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CDialogExtendedProgress can't create window class from Kodi !!!\n");
  }

  CDialogExtendedProgress::~CDialogExtendedProgress()
  {
    if (m_DialogHandle)
      g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.Delete(g_interProcess.m_Handle, m_DialogHandle);
  }

  std::string CDialogExtendedProgress::Title() const
  {
    std::string text;
    text.resize(1024);
    unsigned int size = (unsigned int)text.capacity();
    g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.Title(g_interProcess.m_Handle, m_DialogHandle, text[0], size);
    text.resize(size);
    text.shrink_to_fit();
    return text;
  }

  void CDialogExtendedProgress::SetTitle(const std::string& title)
  {
    g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.SetTitle(g_interProcess.m_Handle, m_DialogHandle, title.c_str());
  }

  std::string CDialogExtendedProgress::Text() const
  {
    std::string text;
    text.resize(1024);
    unsigned int size = (unsigned int)text.capacity();
    g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.Text(g_interProcess.m_Handle, m_DialogHandle, text[0], size);
    text.resize(size);
    text.shrink_to_fit();
    return text;
  }

  void CDialogExtendedProgress::SetText(const std::string& text)
  {
    g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.SetText(g_interProcess.m_Handle, m_DialogHandle, text.c_str());
  }

  bool CDialogExtendedProgress::IsFinished() const
  {
    return g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.IsFinished(g_interProcess.m_Handle, m_DialogHandle);
  }

  void CDialogExtendedProgress::MarkFinished()
  {
    g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.MarkFinished(g_interProcess.m_Handle, m_DialogHandle);
  }
    
  float CDialogExtendedProgress::Percentage() const
  {
    return g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.Percentage(g_interProcess.m_Handle, m_DialogHandle);
  }
    
  void CDialogExtendedProgress::SetPercentage(float fPercentage)
  {
    g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.SetPercentage(g_interProcess.m_Handle, m_DialogHandle, fPercentage);
  }
    
  void CDialogExtendedProgress::SetProgress(int currentItem, int itemCount)
  {
    g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.SetProgress(g_interProcess.m_Handle, m_DialogHandle, currentItem, itemCount);
  }

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
