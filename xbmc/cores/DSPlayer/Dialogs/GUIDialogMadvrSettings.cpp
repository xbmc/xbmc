/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
 *
 *      Copyright (C) 2014-2015 Aracnoz
 *      http://github.com/aracnoz/xbmc
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

#include "GUIDialogMadvrSettings.h"
#include "settings/MediaSettings.h"

CGUIDialogMadvrSettings::CGUIDialogMadvrSettings()
  : CGUIDialogMadvrSettingsBase(WINDOW_DIALOG_MADVR, "VideoOSDSettings.xml")
{
}

CGUIDialogMadvrSettings::~CGUIDialogMadvrSettings()
{
}

void CGUIDialogMadvrSettings::SetupView()
{
  CGUIDialogMadvrSettingsBase::SetupView();

  SetHeading(m_label);
}

bool CGUIDialogMadvrSettings::OnBack(int actionID)
{
  ReturnToSection();
  return CGUIDialogMadvrSettingsBase::OnBack(actionID);
}

void CGUIDialogMadvrSettings::OnCancel()
{
  ReturnToSection();
  CGUIDialogMadvrSettingsBase::OnCancel();
}

void CGUIDialogMadvrSettings::ReturnToSection()
{
  SaveControlStates();

  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();

  if (madvrSettings.m_sections[m_iSectionId].parentId > MADVR_VIDEO_ROOT)
  {
    m_iSectionId = madvrSettings.m_sections[m_iSectionId].parentId;
    m_label = madvrSettings.m_sections[m_iSectionId].label;
    Close();
    Open();
  }
  else
  {
    SetSection(MADVR_VIDEO_ROOT);
    Close();
  }
}

