/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogBoxBase.h"

#include "guilib/GUIMessage.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <mutex>

#define CONTROL_HEADING 1
#define CONTROL_LINES_START 2
#define CONTROL_TEXTBOX     9

CGUIDialogBoxBase::CGUIDialogBoxBase(int id, const std::string &xmlFile)
    : CGUIDialog(id, xmlFile)
{
  m_bConfirmed = false;
  m_loadType = KEEP_IN_MEMORY;
  m_hasTextbox = false;
}

CGUIDialogBoxBase::~CGUIDialogBoxBase(void) = default;

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
  std::unique_lock<CCriticalSection> lock(m_section);
  if (label != m_strHeading)
  {
    m_strHeading = label;
    SetInvalid();
  }
}

bool CGUIDialogBoxBase::HasHeading() const
{
  std::unique_lock<CCriticalSection> lock(m_section);
  return !m_strHeading.empty();
}

void CGUIDialogBoxBase::SetLine(unsigned int iLine, const CVariant& line)
{
  std::string label = GetLocalized(line);
  std::unique_lock<CCriticalSection> lock(m_section);
  std::vector<std::string> lines = StringUtils::Split(m_text, '\n');
  if (iLine >= lines.size())
    lines.resize(iLine+1);
  lines[iLine] = label;
  std::string text = StringUtils::Join(lines, "\n");
  SetText(text);
}

void CGUIDialogBoxBase::SetText(const CVariant& text)
{
  std::string label = GetLocalized(text);
  std::unique_lock<CCriticalSection> lock(m_section);
  StringUtils::Trim(label, "\n");
  if (label != m_text)
  {
    m_text = label;
    SetInvalid();
  }
}

bool CGUIDialogBoxBase::HasText() const
{
  std::unique_lock<CCriticalSection> lock(m_section);
  return !m_text.empty();
}

void CGUIDialogBoxBase::SetChoice(int iButton, const CVariant &choice) // iButton == 0 for no, 1 for yes
{
  if (iButton < 0 || iButton >= DIALOG_MAX_CHOICES)
    return;

  std::string label = GetLocalized(choice);
  std::unique_lock<CCriticalSection> lock(m_section);
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
    std::string heading, text;
    std::vector<std::string> choices;
    choices.reserve(DIALOG_MAX_CHOICES);
    {
      std::unique_lock<CCriticalSection> lock(m_section);
      heading = m_strHeading;
      text = m_text;
      for (const std::string& choice : m_strChoices)
        choices.push_back(choice);
    }
    SET_CONTROL_LABEL(CONTROL_HEADING, heading);
    if (m_hasTextbox)
    {
      SET_CONTROL_LABEL(CONTROL_TEXTBOX, text);
    }
    else
    {
      std::vector<std::string> lines = StringUtils::Split(text, "\n", DIALOG_MAX_LINES);
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
    std::unique_lock<CCriticalSection> lock(m_section);
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
    std::unique_lock<CCriticalSection> lock(m_section);
    m_strHeading.clear();
    m_text.clear();
    for (std::string& choice : m_strChoices)
      choice.clear();
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
