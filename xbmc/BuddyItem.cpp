/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "BuddyItem.h"
#include "utils/KaiClient.h"


#define PING_OFFSETX  8
#define PING_MAX_RATING  4
#define PING_SPACING  2
#define PING_MAX_LATENCY 280

CGUIImage* CBuddyItem::m_pTalkingIcon = NULL;
CGUIImage* CBuddyItem::m_pHeadsetIcon = NULL;
CGUIImage* CBuddyItem::m_pPingIcon = NULL;
CGUIImage* CBuddyItem::m_pInviteIcon = NULL;
CGUIImage* CBuddyItem::m_pBusyIcon = NULL;
CGUIImage* CBuddyItem::m_pIdleIcon = NULL;
CGUIImage* CBuddyItem::m_pHostIcon = NULL;
CGUIImage* CBuddyItem::m_pKeyboardIcon = NULL;

CBuddyItem::CBuddyItem(CStdString& strLabel) : CKaiItem(strLabel)
{
  SetCookie( CKaiClient::Player );

  m_strVector = "/";
  m_strGeoLocation = "";

  m_bIsContact = FALSE;
  m_bIsOnline = FALSE;
  m_bBusy = FALSE;
  m_bKeyboard = FALSE;
  m_bHeadset = FALSE; // does this buddy have a headset connected?
  m_bSpeex = FALSE; // have you enabled speex for this buddy?
  m_bIsTalking = FALSE; // is this buddy chatting?
  m_bProfileRequested = FALSE;

  m_dwPing = 0;

  m_dwSpeexCounter = 0;
  m_dwRingCounter = 0;
  m_bRingIndicator = TRUE;
  m_bInvite = FALSE;

  m_nStatus = 0;
}

CBuddyItem::~CBuddyItem(void)
{}

CStdString CBuddyItem::GetArena()
{
  INT arenaDelimiter = m_strVector.ReverseFind('/') + 1;
  return m_strVector.Mid(arenaDelimiter);
}

void CBuddyItem::SetIcons(float width, float height, const CStdString& aHeadsetTexture,
                          const CStdString& aChatTexture, const CStdString& aPingTexture,
                          const CStdString& aInviteTexture, const CStdString& aBusyTexture,
                          const CStdString& aIdleTexture, const CStdString& aHostTexture,
                          const CStdString& aKeyboardTexture)
{
  if (aChatTexture.length() > 0)
  {
    m_pTalkingIcon = new CGUIImage(0, 0, 0, 0, width, height, aChatTexture, 0x0);
    m_pTalkingIcon->AllocResources();
  }

  if (aHeadsetTexture.length() > 0)
  {
    m_pHeadsetIcon = new CGUIImage(0, 0, 0, 0, width, height, aHeadsetTexture, 0x0);
    m_pHeadsetIcon->AllocResources();
  }

  if (aPingTexture.length())
  {
    m_pPingIcon = new CGUIImage(0, 0, 0, 0, width, height, aPingTexture, 0x0);
    m_pPingIcon->AllocResources();
  }

  if (aInviteTexture.length())
  {
    m_pInviteIcon = new CGUIImage(0, 0, 0, 0, width, height, aInviteTexture, 0x0);
    m_pInviteIcon->AllocResources();
  }

  if (aBusyTexture.length())
  {
    m_pBusyIcon = new CGUIImage(0, 0, 0, 0, width, height, aBusyTexture, 0x0);
    m_pBusyIcon->AllocResources();
  }

  if (aIdleTexture.length())
  {
    m_pIdleIcon = new CGUIImage(0, 0, 0, 0, width, height, aIdleTexture, 0x0);
    m_pIdleIcon->AllocResources();
  }

  if (aHostTexture.length())
  {
    m_pHostIcon = new CGUIImage(0, 0, 0, 0, width, height, aHostTexture, 0x0);
    m_pHostIcon->AllocResources();
  }

  if (aKeyboardTexture.length())
  {
    m_pKeyboardIcon = new CGUIImage(0, 0, 0, 0, width, height, aKeyboardTexture, 0x0);
    m_pKeyboardIcon->AllocResources();
  }
}

void CBuddyItem::FreeIcons()
{
  if (m_pTalkingIcon)
  {
    m_pTalkingIcon->FreeResources();
    delete m_pTalkingIcon;
    m_pTalkingIcon = NULL;
  }

  if (m_pHeadsetIcon)
  {
    m_pHeadsetIcon->FreeResources();
    delete m_pHeadsetIcon;
    m_pHeadsetIcon = NULL;
  }

  if (m_pPingIcon)
  {
    m_pPingIcon->FreeResources();
    delete m_pPingIcon;
    m_pPingIcon = NULL;
  }

  if (m_pInviteIcon)
  {
    m_pInviteIcon->FreeResources();
    delete m_pInviteIcon;
    m_pInviteIcon = NULL;
  }

  if (m_pBusyIcon)
  {
    m_pBusyIcon->FreeResources();
    delete m_pBusyIcon;
    m_pBusyIcon = NULL;
  }

  if (m_pIdleIcon)
  {
    m_pIdleIcon->FreeResources();
    delete m_pIdleIcon;
    m_pIdleIcon = NULL;
  }

  if (m_pHostIcon)
  {
    m_pHostIcon->FreeResources();
    delete m_pHostIcon;
    m_pHostIcon = NULL;
  }

  if (m_pKeyboardIcon)
  {
    m_pKeyboardIcon->FreeResources();
    delete m_pKeyboardIcon;
    m_pKeyboardIcon = NULL;
  }
}

void CBuddyItem::OnPaint(CGUIItem::RenderContext* pContext)
{
  CKaiItem::OnPaint(pContext);

  CGUIListExItem::RenderContext* pDC = (CGUIListExItem::RenderContext*)pContext;

  if (pDC && m_pPingIcon && m_pHeadsetIcon && m_pTalkingIcon && m_pInviteIcon)
  {
    float endButtonPosX = pDC->m_positionX + pDC->m_pButton->GetWidth();
    float pingPosY = (float)pDC->m_positionY;
    float pingPosX = endButtonPosX;

    pingPosY += (pDC->m_pButton->GetHeight() - m_pPingIcon->GetHeight()) / 2;
    pingPosX -= ( (m_pPingIcon->GetWidth() + PING_SPACING) * PING_MAX_RATING ) + PING_OFFSETX;

    float headsetIconPosX = pingPosX - (m_pHeadsetIcon->GetWidth() + 4);
    float inviteIconPosX = headsetIconPosX - (m_pInviteIcon->GetWidth() + 2);

    // if buddy has been talking for the last second
    if (m_dwSpeexCounter > 0)
    {
      m_dwSpeexCounter--;
      m_pTalkingIcon->SetPosition(headsetIconPosX, pingPosY);
      m_pTalkingIcon->Render();
    }
    // if we have enabled voice chat for this buddy
    else if (m_bSpeex)
    {
      m_pHeadsetIcon->SetAlpha(255);
      m_pHeadsetIcon->SetPosition(headsetIconPosX, pingPosY);
      m_pHeadsetIcon->Render();
    }
    // if buddy is trying to establish voice chat
    else if (m_dwRingCounter > 0)
    {
      m_dwRingCounter--;
      // flash headset icon
      if (m_dwFrameCounter % 60 >= 30)
      {
        m_pHeadsetIcon->SetAlpha(255);
        m_pHeadsetIcon->SetPosition(headsetIconPosX, pingPosY);
        m_pHeadsetIcon->Render();
      }
    }
    // if buddy has a headset
    else if (m_bHeadset)
    {
      m_pHeadsetIcon->SetAlpha(95);
      m_pHeadsetIcon->SetPosition(headsetIconPosX, pingPosY);
      m_pHeadsetIcon->Render();
    }
    // if buddy has a headset
    else if (m_bKeyboard && m_pKeyboardIcon)
    {
      m_pKeyboardIcon->SetPosition(headsetIconPosX, pingPosY);
      m_pKeyboardIcon->Render();
    }
    // if buddy has sent an invite
    if (m_bInvite)
    {
      if (m_dwFrameCounter % 60 >= 30)
      {
        // flash invitation icon
        m_pInviteIcon->SetPosition(inviteIconPosX, pingPosY);
        m_pInviteIcon->Render();
      }
    }
    else if (m_bIsOnline)
    {
      if (m_nStatus == 0 && m_bBusy && m_pIdleIcon) // idle
      {
        m_pIdleIcon->SetPosition(inviteIconPosX, pingPosY);
        m_pIdleIcon->Render();
      }
      else if (m_nStatus == 1 && m_pBusyIcon) // joining
      {
        m_pBusyIcon->SetPosition(inviteIconPosX, pingPosY);
        m_pBusyIcon->Render();
      }
      else if (m_nStatus > 1 && m_pHostIcon)  // hosting
      {
        m_pHostIcon->SetPosition(inviteIconPosX, pingPosY);
        m_pHostIcon->Render();
      }
    }

    if (m_pPingIcon && m_dwPing > 0 && m_dwPing <= PING_MAX_LATENCY )
    {
      DWORD dwPingAlpha = 95;
      DWORD dwStep = PING_MAX_LATENCY / PING_MAX_RATING;
      DWORD dwRating = PING_MAX_RATING - (m_dwPing / dwStep);
      float posX = pingPosX;
      for (DWORD dwGraduation = 0; dwGraduation < dwRating; dwGraduation++)
      {
        dwPingAlpha += 40;
        m_pPingIcon->SetPosition(posX, pingPosY);
        m_pPingIcon->SetAlpha((unsigned char)dwPingAlpha);
        m_pPingIcon->Render();

        posX += m_pPingIcon->GetWidth() + PING_SPACING;
      }
    }
  }
}