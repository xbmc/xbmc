/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "guilib/LocalizeStrings.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"

using namespace std;

#define CONTROL_HEADING 1
#define CONTROL_LINES_START 2
#define CONTROL_TEXTBOX     9
#define CONTROL_CHOICES_START 10

CGUIDialogBoxBase::CGUIDialogBoxBase(int id, const std::string &xmlFile)
    : CGUIDialog(id, xmlFile)
{
  m_bConfirmed = false;
  m_loadType = KEEP_IN_MEMORY;
  m_hasTextbox = false;
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
  std::string label = GetLocalized(heading);
  CSingleLock lock(m_section);
  if (label != m_strHeading)
  {
    m_strHeading = label;
    SetInvalid();
  }
}

void CGUIDialogBoxBase::SetLine(unsigned int iLine, const CVariant& line)
{
  std::string label = GetLocalized(line);
  CSingleLock lock(m_section);
  vector<string> lines = StringUtils::Split(m_text, '\n');
  if (iLine >= lines.size())
    lines.resize(iLine+1);
  lines[iLine] = label;
  std::string text = StringUtils::Join(lines, "\n");
  SetText(text);
}

void CGUIDialogBoxBase::SetText(const CVariant& text)
{
  std::string label = GetLocalized(text);
  CSingleLock lock(m_section);
  StringUtils::Trim(label, "\n");
  if (label != m_text)
  {
    m_text = label;
    SetInvalid();
  }
}

void CGUIDialogBoxBase::SetChoice(int iButton, const CVariant &choice) // iButton == 0 for no, 1 for yes
{
  if (iButton < 0 || iButton >= DIALOG_MAX_CHOICES)
    return;

  std::string label = GetLocalized(choice);
  CSingleLock lock(m_section);
  if (label != m_strChoices[iButton])
  {
    m_strChoices[iButton] = label;
    SetInvalid();
  }
}

void CGUIDialogBoxBase::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_bInvalidated)
  { // take a copy of our labels to save holding the lock for too long
    string heading, text;
    vector<string> choices;
    choices.reserve(DIALOG_MAX_CHOICES);
    {
      CSingleLock lock(m_section);
      heading = m_strHeading;
      text = m_text;
      for (int i = 0; i < DIALOG_MAX_CHOICES; ++i)
        choices.push_back(m_strChoices[i]);
    }
    SET_CONTROL_LABEL(CONTROL_HEADING, heading);
    if (m_hasTextbox)
    {
      SET_CONTROL_LABEL(CONTROL_TEXTBOX, text);
    }
    else
    {
      vector<string> lines = StringUtils::Split(text, "\n", DIALOG_MAX_LINES);
      lines.resize(DIALOG_MAX_LINES);
      for (size_t i = 0 ; i < lines.size(); ++i)
        SET_CONTROL_LABEL(CONTROL_LINES_START + i, lines[i]);
    }
    for (size_t i = 0 ; i < choices.size() ; ++i)
      SET_CONTROL_LABEL(CONTROL_CHOICES_START + i, choices[i]);
  }
  CGUIDialog::Process(currentTime, dirtyregions);
}

void CGUIDialogBoxBase::OnInitWindow()
{
  // set focus to default
  m_lastControlID = m_defaultControl;

  m_hasTextbox = false;
  const CGUIControl *control = GetControl(CONTROL_TEXTBOX);
  if (control && control->GetControlType() == CGUIControl::GUICONTROL_TEXTBOX)
    m_hasTextbox = true;

  // set initial labels
  {
    CSingleLock lock(m_section);
    for (int i = 0 ; i < DIALOG_MAX_CHOICES ; ++i)
    {
      if (m_strChoices[i].empty())
        m_strChoices[i] = GetDefaultLabel(CONTROL_CHOICES_START + i);
    }
  }
  CGUIDialog::OnInitWindow();
}

void CGUIDialogBoxBase::OnDeinitWindow(int nextWindowID)
{
  // make sure we set default labels for heading, lines and choices
  {
    CSingleLock lock(m_section);
    m_strHeading.clear();
    m_text.clear();
    for (int i = 0 ; i < DIALOG_MAX_CHOICES ; ++i)
      m_strChoices[i].clear();
  }

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

std::string CGUIDialogBoxBase::GetLocalized(const CVariant &var) const
{
  if (var.isString())
    return var.asString();
  else if (var.isInteger() && var.asInteger())
    return g_localizeStrings.Get((uint32_t)var.asInteger());
  return "";
}

std::string CGUIDialogBoxBase::GetDefaultLabel(int controlId) const
{
  int labelId = GetDefaultLabelID(controlId);
  return labelId != -1 ? g_localizeStrings.Get(labelId) : "";
}

int CGUIDialogBoxBase::GetDefaultLabelID(int controlId) const
{
  return -1;
}
