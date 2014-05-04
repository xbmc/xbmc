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

#include "KeyboardLayout.h"

CKeyboardLayout::CKeyboardLayout(const std::string &name)
{
  m_strName = name;
}

CKeyboardLayout::~CKeyboardLayout(void)
{

}

std::string CKeyboardLayout::GetName() const
{
  return m_strName;
}

void CKeyboardLayout::AddKeyboardRow(const std::wstring &wstrKeyboardRow, unsigned int iModifierKeys)
{
  m_keyboards[iModifierKeys].push_back(wstrKeyboardRow);
}

WCHAR CKeyboardLayout::GetCharAt(unsigned int iRow, unsigned int iColumn, unsigned int iModifierKeys)
{
  KeyboardRows rows = m_keyboards[iModifierKeys];
  if (iModifierKeys != MODIFIER_KEY_NONE && rows.empty())
  {
    // fallback to basic keyboard
    rows = m_keyboards[MODIFIER_KEY_NONE];
  }

  if (iRow < rows.size())
  {
    if (iColumn < rows[iRow].size())
    {
      WCHAR ch = rows[iRow][iColumn];
      if (ch != L' ')
        return ch;
    }
  }

  return L'\0';
}

void CKeyboardLayout::Clear()
{
  for (Keyboards::const_iterator it = m_keyboards.begin(); it != m_keyboards.end(); it++)
  {
    KeyboardRows rows = it->second;
    rows.clear();
  }
  m_keyboards.clear();
}
