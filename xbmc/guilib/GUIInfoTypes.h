/*!
\file GUIInfoTypes.h
\brief
*/

#ifndef GUILIB_GUIINFOTYPES_H
#define GUILIB_GUIINFOTYPES_H

#pragma once

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

#include "utils/StdString.h"

class CGUIListItem;

class CGUIInfoBool
{
public:
  CGUIInfoBool(bool value = false);
  virtual ~CGUIInfoBool();

  operator bool() const { return m_value; };

  void Update(const CGUIListItem *item = NULL);
  void Parse(const CStdString &expression, int context);
private:
  unsigned int m_info;
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

  bool Update();
  void Parse(const CStdString &label, int context);

private:
  color_t GetColor() const;
  int     m_info;
  color_t m_color;
};

class CGUIInfoLabel
{
public:
  CGUIInfoLabel();
  CGUIInfoLabel(const CStdString &label, const CStdString &fallback = "", int context = 0);

  void SetLabel(const CStdString &label, const CStdString &fallback, int context = 0);

  /*!
   \brief Gets a label (or image) for a given window context from the info manager.
   \param contextWindow the context in which to evaluate the expression.
   \param preferImage caller is specifically wanting an image rather than a label. Defaults to false.
   \param fallback if non-NULL, is set to an alternate value to use should the actual value be not appropriate. Defaults to NULL.
   \return label (or image).
   */  
  CStdString GetLabel(int contextWindow, bool preferImage = false, CStdString *fallback = NULL) const;

  /*!
   \brief Gets a label (or image) for a given listitem from the info manager.
   \param item listitem in question.
   \param preferImage caller is specifically wanting an image rather than a label. Defaults to false.
   \param fallback if non-NULL, is set to an alternate value to use should the actual value be not appropriate. Defaults to NULL.
   \return label (or image).
   */
  CStdString GetItemLabel(const CGUIListItem *item, bool preferImage = false, CStdString *fallback = NULL) const;

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
  void Parse(const CStdString &label, int context);

  class CInfoPortion
  {
  public:
    CInfoPortion(int info, const CStdString &prefix, const CStdString &postfix, bool escaped = false);
    CStdString GetLabel(const CStdString &info) const;
    int m_info;
    CStdString m_prefix;
    CStdString m_postfix;
  private:
    bool m_escaped;
  };

  CStdString m_fallback;
  std::vector<CInfoPortion> m_info;
};

#endif
