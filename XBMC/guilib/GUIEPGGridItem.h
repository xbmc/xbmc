/*!
\file GUIEPGGridItem.h
\brief 
*/

#ifndef GUILIB_GUIEPGGRIDITEM_H
#define GUILIB_GUIEPGGRIDITEM_H

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

#include <map>
#include <string>

//  Forward
class CGUIEPGGridItemLayout;

/*!
 \ingroup controls
 \brief 
 */
class CGUIEPGGridItem
{
public:
  enum GUIEPGGridItemOverlay { ICON_OVERLAY_NONE = 0,
                               ICON_OVERLAY_RECORDING,
                               ICON_OVERLAY_AUTOWATCH};
  CGUIEPGGridItem(void);
  CGUIEPGGridItem(const CGUIEPGGridItem& item);
  virtual ~CGUIEPGGridItem(void);

  const CGUIEPGGridItem& operator =(const CGUIEPGGridItem& item);

  virtual void SetLabel(const CStdString& strLabel);
  const CStdString& GetLabel() const;

  void SetGridItemBG(const CStdString& strImage);
  const CStdString& GetGridItemBG() const;

  void SetOverlayImage(GUIEPGGridItemOverlay icon, bool bOnOff=false);
  CStdString GetOverlayImage() const;

  void Select(bool bOnOff);
  bool IsSelected() const;
  
  bool HasOverlay() const;

  void SetLayout(CGUIEPGGridItemLayout *layout);
  CGUIEPGGridItemLayout *GetLayout();

  void SetFocusedLayout(CGUIEPGGridItemLayout *layout);
  CGUIEPGGridItemLayout *GetFocusedLayout();

  void FreeMemory();
  void SetInvalid();

  void SetProperty(const CStdString &strKey, const char *strValue);
  void SetProperty(const CStdString &strKey, const CStdString &strValue);
  void SetProperty(const CStdString &strKey, int nVal);
  void SetProperty(const CStdString &strKey, bool bVal);
  void SetProperty(const CStdString &strKey, double dVal);
  void ClearProperties();

  bool       HasProperty(const CStdString &strKey) const;
  void       ClearProperty(const CStdString &strKey);

  CStdString GetProperty(const CStdString &strKey) const;
  bool       GetPropertyBOOL(const CStdString &strKey) const;
  int        GetPropertyInt(const CStdString &strKey) const;
  double     GetPropertyDouble(const CStdString &strKey) const;

protected:
  CStdString m_strLabel;      // programme title
  GUIEPGGridItemOverlay m_overlayIcon; // type of overlay icon
  
  CGUIEPGGridItemLayout *m_layout;
  CGUIEPGGridItemLayout *m_focusedLayout;
  bool m_bSelected;

  struct icompare
  {
    bool operator()(const CStdString &s1, const CStdString &s2) const
    {
      return s1.CompareNoCase(s2) < 0;
    }
  };

  std::map<CStdString, CStdString, icompare> m_mapProperties;
};
#endif
