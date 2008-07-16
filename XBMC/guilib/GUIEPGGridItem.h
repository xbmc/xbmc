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
#include "datetime.h"
#include "GUIListItem.h"

#include <map>
#include <string>

//  Forward
class CGUIListItemLayout;
class CGUIListItem;

/*!
 \ingroup controls
 \brief 
 */
class CGUIEPGGridItem : public CGUIListItem
{
public:
  enum GUIEPGGridItemOverlay { ICON_OVERLAY_NONE = 0,
                               ICON_OVERLAY_RECORDING,
                               ICON_OVERLAY_AUTOWATCH};
  CGUIEPGGridItem(void);
  virtual ~CGUIEPGGridItem(void);

  const CGUIEPGGridItem& operator =(const CGUIEPGGridItem& item);

  void SetLabel(const CStdString& strLabel);
  const CStdString& GetLabel() const;

  void SetShortDesc(const CStdString& strShortDesc);
  const CStdString& GetShortDesc() const;

  void SetStartTime(const CStdString& startTime);
  const CDateTime& GetStartTime() const;

  void SetDuration(const int& duration);
  const int& GetDuration() const;

  void SetOverlayImage(GUIEPGGridItemOverlay icon, bool bOnOff=false);
  CStdString GetOverlayImage() const;

  void SetLayout(CGUIListItemLayout *layout);
  CGUIListItemLayout *GetLayout();

  void SetFocusedLayout(CGUIListItemLayout *layout);
  CGUIListItemLayout *GetFocusedLayout();

protected:
  GUIEPGGridItemOverlay m_overlayIcon; // type of overlay icon

  CStdString m_pCategory;
  CStdString m_pShortDesc;
  CStdString m_pLongDesc;
  CDateTime  m_pStartTime;
  int        m_pDuration;
};
#endif
