/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "Application.h"
#include "GUIDialogBoxBase.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"

using namespace std;

#define CONTROL_HEADING 1
#define CONTROL_LINES_START 2
#define CONTROL_CHOICES_START 10

CGUIDialogBoxBase::CGUIDialogBoxBase(int id, const CStdString &xmlFile)
    : CGUIDialog(id, xmlFile)
{
  m_bConfirmed = false;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogBoxBase::~CGUIDialogBoxBase(void)
{
}

bool CGUIDialogBoxBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      m_bConfirmed = false;
      return true;
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxBase::IsConfirmed() const
{
  return m_bConfirmed;
}

void CGUIDialogBoxBase::SetHeading(const CVariant& heading)
{
  m_strHeading = GetLocalized(heading);
  if (IsActive())
    SET_CONTROL_LABEL_THREAD_SAFE(1, m_strHeading);
}

void CGUIDialogBoxBase::SetLine(int iLine, const CVariant& line)
{
  if (iLine < 0 || iLine >= DIALOG_MAX_LINES)
    return;

  m_strLines[iLine] = GetLocalized(line);
  if (IsActive())
    SET_CONTROL_LABEL_THREAD_SAFE(CONTROL_LINES_START + iLine, m_strLines[iLine]);
}

void CGUIDialogBoxBase::SetChoice(int iButton, const CVariant &choice) // iButton == 0 for no, 1 for yes
{
  if (iButton < 0 || iButton >= DIALOG_MAX_CHOICES)
    return;

  m_strChoices[iButton] = GetLocalized(choice);
  if (IsActive())
    SET_CONTROL_LABEL_THREAD_SAFE(CONTROL_CHOICES_START + iButton, m_strChoices[iButton]);
}

void CGUIDialogBoxBase::OnInitWindow()
{
  // set focus to default
  m_lastControlID = m_defaultControl;

  // set control labels
  SET_CONTROL_LABEL(CONTROL_HEADING, !m_strHeading.empty() ? m_strHeading : GetDefaultLabel(CONTROL_HEADING));
  for (int i = 0 ; i < DIALOG_MAX_LINES ; ++i)
    SET_CONTROL_LABEL(CONTROL_LINES_START + i, !m_strLines[i].empty() ? m_strLines[i] : GetDefaultLabel(CONTROL_LINES_START + i));
  for (int i = 0 ; i < DIALOG_MAX_CHOICES ; ++i)
    SET_CONTROL_LABEL(CONTROL_CHOICES_START + i, !m_strChoices[i].empty() ? m_strChoices[i] : GetDefaultLabel(CONTROL_CHOICES_START + i));

  CGUIDialog::OnInitWindow();
}

void CGUIDialogBoxBase::OnDeinitWindow(int nextWindowID)
{
  // make sure we set default labels for heading, lines and choices
  SetHeading(m_strHeading = "");
  for (int i = 0 ; i < DIALOG_MAX_LINES ; ++i)
    SetLine(i, m_strLines[i] = "");
  for (int i = 0 ; i < DIALOG_MAX_CHOICES ; ++i)
    SetChoice(i, m_strChoices[i] = "");

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

CStdString CGUIDialogBoxBase::GetLocalized(const CVariant &var) const
{
  if (var.isString())
    return var.asString();
  else if (var.isInteger() && var.asInteger())
    return g_localizeStrings.Get((uint32_t)var.asInteger());
  return "";
}

CStdString CGUIDialogBoxBase::GetDefaultLabel(int controlId) const
{
  int labelId = GetDefaultLabelID(controlId);
  return labelId != -1 ? g_localizeStrings.Get(labelId) : "";
}

int CGUIDialogBoxBase::GetDefaultLabelID(int controlId) const
{
  return -1;
}
