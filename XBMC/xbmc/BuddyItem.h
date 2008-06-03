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

class CBuddyItem : public CKaiItem
{
public:
  CBuddyItem(CStdString& strLabel);
  virtual ~CBuddyItem(void);
  virtual void OnPaint(CGUIItem::RenderContext* pContext);
  static void SetIcons(float width, float height, const CStdString& aHeadsetTexture,
                       const CStdString& aChatTexture, const CStdString& aPingTexture,
                       const CStdString& aInviteTexture, const CStdString& aBusyTexture,
                       const CStdString& aIdleTexture, const CStdString& aHostTexture,
                       const CStdString& aKeyboardTexture);
  static void FreeIcons();

  CStdString GetArena();

public:
  CStdString m_strVector;
  CStdString m_strGeoLocation;
  DWORD m_dwPing;
  bool m_bIsOnline;
  bool m_bSpeex;
  bool m_bBusy;
  bool m_bKeyboard;
  bool m_bHeadset;
  bool m_bIsTalking;
  bool m_bIsContact;
  bool m_bProfileRequested;

  DWORD m_dwSpeexCounter;
  DWORD m_dwRingCounter;
  bool m_bInvite;
  bool m_bRingIndicator;

  int m_nStatus;

protected:

  static CGUIImage* m_pPingIcon;
  static CGUIImage* m_pTalkingIcon;
  static CGUIImage* m_pHeadsetIcon;
  static CGUIImage* m_pInviteIcon;
  static CGUIImage* m_pBusyIcon;
  static CGUIImage* m_pIdleIcon;
  static CGUIImage* m_pHostIcon;
  static CGUIImage* m_pKeyboardIcon;
};

