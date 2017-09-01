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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GameWindowFullScreenText.h"
#include "video/windows/GUIWindowFullScreenDefines.h"
#include "guilib/GUIDialog.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindow.h"

using namespace KODI;
using namespace RETRO;

CGameWindowFullScreenText::CGameWindowFullScreenText(CGUIWindow &fullscreenWindow) :
  m_fullscreenWindow(fullscreenWindow)
{
}

void CGameWindowFullScreenText::OnWindowLoaded()
{
  m_bShowText = false;
  m_bTextChanged = true;
  m_bTextVisibilityChanged = true;
  m_lines.clear();
}

void CGameWindowFullScreenText::FrameMove()
{
  if (m_bTextChanged)
  {
    m_bTextChanged = false;
    UploadText();
  }

  if (m_bTextVisibilityChanged)
  {
    m_bTextVisibilityChanged = false;

    if (m_bShowText)
      Show();
    else
      Hide();
  }
}

const std::string &CGameWindowFullScreenText::GetText(unsigned int lineIndex) const
{
  if (lineIndex < m_lines.size())
    return m_lines[lineIndex];

  static const std::string empty;
  return empty;
}

void CGameWindowFullScreenText::SetText(unsigned int lineIndex, std::string line)
{
  if (lineIndex >= m_lines.size())
    m_lines.resize(lineIndex + 1);

  m_lines[lineIndex] = std::move(line);
}

const std::vector<std::string> &CGameWindowFullScreenText::GetText() const
{
  return m_lines;
}

void CGameWindowFullScreenText::SetText(std::vector<std::string> text)
{
  m_lines = std::move(text);
}

void CGameWindowFullScreenText::UploadText()
{
  for (unsigned int i = 0; i < m_lines.size(); i++)
  {
    int rowControl = GetControlID(i);
    if (rowControl > 0)
      SET_CONTROL_LABEL(rowControl, m_lines[i]);
  }
}

void CGameWindowFullScreenText::Show()
{
  SET_CONTROL_VISIBLE(LABEL_ROW1);
  SET_CONTROL_VISIBLE(LABEL_ROW2);
  SET_CONTROL_VISIBLE(LABEL_ROW3);
  SET_CONTROL_VISIBLE(BLUE_BAR);
}

void CGameWindowFullScreenText::Hide()
{
  SET_CONTROL_HIDDEN(LABEL_ROW1);
  SET_CONTROL_HIDDEN(LABEL_ROW2);
  SET_CONTROL_HIDDEN(LABEL_ROW3);
  SET_CONTROL_HIDDEN(BLUE_BAR);
}

int CGameWindowFullScreenText::GetID() const
{
  return m_fullscreenWindow.GetID();
}

bool CGameWindowFullScreenText::OnMessage(CGUIMessage &message)
{
  return m_fullscreenWindow.OnMessage(message);
}

int CGameWindowFullScreenText::GetControlID(unsigned int lineIndex)
{
  switch (lineIndex)
  {
  case 0: return LABEL_ROW1;
  case 1: return LABEL_ROW2;
  case 2: return LABEL_ROW3;
  default:
    break;
  }

  return -1;
}
