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

#include "KaiItem.h"

class CArenaItem : public CKaiItem
{
public:
  enum Tier {Root = 0, Platform = 1, Genre = 2, Game = 3, Custom = 4};

  CArenaItem(CStdString& strLabel);
  virtual ~CArenaItem(void);

  Tier GetTier();
  virtual void OnPaint(CGUIItem::RenderContext* pContext);
  virtual void GetDisplayText(CStdString& aString);

  void GetTier(CArenaItem::Tier aTier, CStdString& aTierName);

  static void GetTier(Tier aTier, CStdString aVector, CStdString& aTierName);
  static void SetIcons(float width, float height, const CStdString& aHeadsetTexture);
  static void FreeIcons();

  CStdString m_strVector;
  CStdString m_strDescription;
  CStdString m_strPassword; // arena password - cached

  int m_nPlayers;
  int m_nPlayerLimit;
  bool m_bIsPersonal; // created by another player
  bool m_bIsPrivate; // requires a password on entry
protected:
  static CGUIImage* m_pPrivateIcon;
};
