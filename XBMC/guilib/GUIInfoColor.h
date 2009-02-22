/*!
\file GUIInfoColor.h
\brief 
*/

#ifndef GUILIB_GUIINFOCOLOR_H
#define GUILIB_GUIINFOCOLOR_H

#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

class CGUIListItem;

class CGUIInfoBool
{
public:
  CGUIInfoBool(bool value = false);
  operator bool() const { return m_value; };

  void Update(DWORD parentID = 0, const CGUIListItem *item = NULL);
  void Parse(const CStdString &info);
private:
  int m_info;
  bool m_value;
};

class CGUIInfoColor
{
public:
  CGUIInfoColor(DWORD color = 0);

  const CGUIInfoColor &operator=(const CGUIInfoColor &color);
  const CGUIInfoColor &operator=(DWORD color);
  operator DWORD() const { return m_color; };

  void Update();
  void Parse(const CStdString &label);

private:
  DWORD GetColor() const;
  int m_info;
  DWORD m_color;
};

#endif
