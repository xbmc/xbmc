/*!
\file GUIInfoTypes.h
\brief
*/

#ifndef GUILIB_GUIINFOTYPES_H
#define GUILIB_GUIINFOTYPES_H

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

#include "StdString.h"

class CGUIListItem;

class CGUIInfoBool
{
public:
  CGUIInfoBool(bool value = false);
  operator bool() const { return m_value; };

  void Update(int parentID = 0, const CGUIListItem *item = NULL);
  void Parse(const CStdString &info);
private:
  int m_info;
  bool m_value;
};

typedef uint32_t color_t;

class CGUIInfoColor
{
public:
  CGUIInfoColor(color_t color = 0);

  const CGUIInfoColor &operator=(const CGUIInfoColor &color);
  const CGUIInfoColor &operator=(color_t color);
  operator color_t() const { return m_color; };

  void Update();
  void Parse(const CStdString &label);

private:
  color_t GetColor() const;
  int     m_info;
  color_t m_color;
};

class CGUIInfoLabel
{
public:
  CGUIInfoLabel();
  CGUIInfoLabel(const CStdString &label, const CStdString &fallback = "");

  void SetLabel(const CStdString &label, const CStdString &fallback);
  CStdString GetLabel(int contextWindow, bool preferImage = false) const;
  CStdString GetItemLabel(const CGUIListItem *item, bool preferImage = false) const;
  bool IsConstant() const;
  bool IsEmpty() const;

  const CStdString GetFallback() const { return m_fallback; };

  static CStdString GetLabel(const CStdString &label, int contextWindow = 0, bool preferImage = false);

  /*!
   \brief Replaces instances of $LOCALIZE[number] with the appropriate localized string
   \param label text to replace
   \return text with any localized strings filled in.
   */
  static CStdString ReplaceLocalize(const CStdString &label);

  /*!
   \brief Replaces instances of $ADDON[id number] with the appropriate localized addon string
   \param label text to replace
   \return text with any localized strings filled in.
   */
  static CStdString ReplaceAddonStrings(const CStdString &label);

private:
  void Parse(const CStdString &label);

  class CInfoPortion
  {
  public:
    CInfoPortion(int info, const CStdString &prefix, const CStdString &postfix);
    int m_info;
    CStdString m_prefix;
    CStdString m_postfix;
  };

  CStdString m_fallback;
  std::vector<CInfoPortion> m_info;
};

#endif
