
#include "StdAfx.h"
#include "BuddyItem.h"
#include "Settings.h"
#include "Utils/KaiClient.h"

#define PING_OFFSETX		8
#define PING_MAX_RATING		4
#define PING_SPACING		2
#define PING_MAX_LATENCY	280

CGUIImage* CBuddyItem::m_pTalkingIcon	= NULL;
CGUIImage* CBuddyItem::m_pHeadsetIcon	= NULL;
CGUIImage* CBuddyItem::m_pPingIcon		= NULL;
CGUIImage* CBuddyItem::m_pInviteIcon	= NULL;
CGUIImage* CBuddyItem::m_pBusyIcon		= NULL;
CGUIImage* CBuddyItem::m_pIdleIcon		= NULL;
CGUIImage* CBuddyItem::m_pHostIcon		= NULL;
CGUIImage* CBuddyItem::m_pKeyboardIcon	= NULL;

CBuddyItem::CBuddyItem(CStdString& strLabel) : CKaiItem(strLabel)
{
	SetCookie( CKaiClient::Item::Player );

	m_strVector			= "/";
	m_strGeoLocation	= "";

	m_bIsContact		= FALSE;
	m_bIsOnline			= FALSE;
	m_bBusy				= FALSE;
	m_bKeyboard			= FALSE;
	m_bHeadset			= FALSE;	// does this buddy have a headset connected?
	m_bSpeex			= FALSE;	// have you enabled speex for this buddy?
	m_bIsTalking		= FALSE;	// is this buddy chatting?
	m_bProfileRequested = FALSE;

	m_dwPing			= 0;

	m_dwSpeexCounter	= 0;
	m_dwRingCounter		= 0;
	m_bRingIndicator	= TRUE;
	m_bInvite			= FALSE;

	m_nStatus			= 0;
}

CBuddyItem::~CBuddyItem(void)
{
}

CStdString CBuddyItem::GetArena()
{
	INT arenaDelimiter = m_strVector.ReverseFind('/')+1;
	return m_strVector.Mid(arenaDelimiter);
}

void CBuddyItem::SetIcons(INT aWidth, INT aHeight, const CStdString& aHeadsetTexture,
							  const CStdString& aChatTexture, const CStdString& aPingTexture,
							  const CStdString& aInviteTexture, const CStdString& aBusyTexture,
							  const CStdString& aIdleTexture, const CStdString& aHostTexture,
							  const CStdString& aKeyboardTexture)
{
	if (aChatTexture.length()>0)
	{
		m_pTalkingIcon = new CGUIImage(0,0,0,0,aWidth,aHeight,aChatTexture,0x0);
		m_pTalkingIcon->AllocResources();
	}

	if (aHeadsetTexture.length()>0)
	{
		m_pHeadsetIcon = new CGUIImage(0,0,0,0,aWidth,aHeight,aHeadsetTexture,0x0);
		m_pHeadsetIcon->AllocResources();
	}

	if (aPingTexture.length())
	{
		m_pPingIcon = new CGUIImage(0,0,0,0,aWidth,aHeight,aPingTexture,0x0);
		m_pPingIcon->AllocResources();
	}

	if (aInviteTexture.length())
	{
		m_pInviteIcon = new CGUIImage(0,0,0,0,aWidth,aHeight,aInviteTexture,0x0);
		m_pInviteIcon->AllocResources();
	}

	if (aBusyTexture.length())
	{
		m_pBusyIcon = new CGUIImage(0,0,0,0,aWidth,aHeight,aBusyTexture,0x0);
		m_pBusyIcon->AllocResources();
	}
	
	if (aIdleTexture.length())
	{
		m_pIdleIcon = new CGUIImage(0,0,0,0,aWidth,aHeight,aIdleTexture,0x0);
		m_pIdleIcon->AllocResources();
	}

	if (aHostTexture.length())
	{
		m_pHostIcon = new CGUIImage(0,0,0,0,aWidth,aHeight,aHostTexture,0x0);
		m_pHostIcon->AllocResources();
	}

	if (aKeyboardTexture.length())
	{
		m_pKeyboardIcon = new CGUIImage(0,0,0,0,aWidth,aHeight,aKeyboardTexture,0x0);
		m_pKeyboardIcon->AllocResources();
	}
}

void CBuddyItem::FreeIcons()
{
	if (m_pTalkingIcon)
	{
		m_pTalkingIcon->FreeResources();
		delete m_pTalkingIcon;
		m_pTalkingIcon=NULL;
	}

	if (m_pHeadsetIcon)
	{
		m_pHeadsetIcon->FreeResources();
		delete m_pHeadsetIcon;
		m_pHeadsetIcon=NULL;
	}

	if (m_pPingIcon)
	{
		m_pPingIcon->FreeResources();
		delete m_pPingIcon;
		m_pPingIcon=NULL;
	}

	if (m_pInviteIcon)
	{
		m_pInviteIcon->FreeResources();
		delete m_pInviteIcon;
		m_pInviteIcon=NULL;
	}

	if (m_pBusyIcon)
	{
		m_pBusyIcon->FreeResources();
		delete m_pBusyIcon;
		m_pBusyIcon=NULL;
	}
	
	if (m_pIdleIcon)
	{
		m_pIdleIcon->FreeResources();
		delete m_pIdleIcon;
		m_pIdleIcon=NULL;
	}

	if (m_pHostIcon)
	{
		m_pHostIcon->FreeResources();
		delete m_pHostIcon;
		m_pHostIcon=NULL;
	}

	if (m_pKeyboardIcon)
	{
		m_pKeyboardIcon->FreeResources();
		delete m_pKeyboardIcon;
		m_pKeyboardIcon=NULL;
	}
}

void CBuddyItem::OnPaint(CGUIItem::RenderContext* pContext)
{
	CKaiItem::OnPaint(pContext);

	CGUIListExItem::RenderContext* pDC = (CGUIListExItem::RenderContext*)pContext;
	
	if (pDC && m_pPingIcon && m_pHeadsetIcon && m_pTalkingIcon && m_pInviteIcon)
	{
		int iEndButtonPosX = pDC->m_iPositionX + pDC->m_pButton->GetWidth();
		int iPingPosY = pDC->m_iPositionY;
		int iPingPosX = iEndButtonPosX;

		iPingPosY += (pDC->m_pButton->GetHeight() - m_pPingIcon->GetHeight())/2;
		iPingPosX -= ( (m_pPingIcon->GetWidth()+PING_SPACING) * PING_MAX_RATING ) + PING_OFFSETX;

		int iHeadsetIconPosX = iPingPosX - (m_pHeadsetIcon->GetWidth() + 4);
		int iInviteIconPosX	 = iHeadsetIconPosX - (m_pInviteIcon->GetWidth() + 2);

		// if buddy has been talking for the last second
		if (m_dwSpeexCounter>0)
		{
			m_dwSpeexCounter--;
			m_pTalkingIcon->SetPosition(iHeadsetIconPosX,iPingPosY);
			m_pTalkingIcon->Render();
		}
		// if we have enabled voice chat for this buddy
		else if (m_bSpeex)
		{
			m_pHeadsetIcon->SetAlpha(255);
			m_pHeadsetIcon->SetPosition(iHeadsetIconPosX,iPingPosY);
			m_pHeadsetIcon->Render();
		}
		// if buddy is trying to establish voice chat
		else if (m_dwRingCounter>0)
		{
			m_dwRingCounter--;
			// flash headset icon
			if (m_dwFrameCounter%60>=30)
			{
				m_pHeadsetIcon->SetAlpha(255);
				m_pHeadsetIcon->SetPosition(iHeadsetIconPosX,iPingPosY);
				m_pHeadsetIcon->Render();
			}
		}
		// if buddy has a headset
		else if (m_bHeadset)
		{
			m_pHeadsetIcon->SetAlpha(95);
			m_pHeadsetIcon->SetPosition(iHeadsetIconPosX,iPingPosY);
			m_pHeadsetIcon->Render();
		}
		// if buddy has a headset
		else if (m_bKeyboard && m_pKeyboardIcon)
		{
			m_pKeyboardIcon->SetPosition(iHeadsetIconPosX,iPingPosY);
			m_pKeyboardIcon->Render();
		}
		// if buddy has sent an invite
		if (m_bInvite)
		{
			if (m_dwFrameCounter%60>=30)
			{
				// flash invitation icon
				m_pInviteIcon->SetPosition(iInviteIconPosX,iPingPosY);
				m_pInviteIcon->Render();
			}
		}
		else if (m_bIsOnline)
		{
			if (m_nStatus==0 && m_bBusy && m_pIdleIcon) // idle
			{
				m_pIdleIcon->SetPosition(iInviteIconPosX,iPingPosY);
				m_pIdleIcon->Render();
			}
			else if (m_nStatus==1 && m_pBusyIcon) // joining
			{
				m_pBusyIcon->SetPosition(iInviteIconPosX,iPingPosY);
				m_pBusyIcon->Render();
			}
			else if (m_nStatus>1 && m_pHostIcon)  // hosting
			{
				m_pHostIcon->SetPosition(iInviteIconPosX,iPingPosY);
				m_pHostIcon->Render();
			}
		}

		if (m_pPingIcon && m_dwPing>0 && m_dwPing<=PING_MAX_LATENCY )
		{
			DWORD dwPingAlpha = 95;
			DWORD dwStep = PING_MAX_LATENCY / PING_MAX_RATING;
			DWORD dwRating = PING_MAX_RATING - (m_dwPing/dwStep);
			int iPosX = iPingPosX;
			for (DWORD dwGraduation=0; dwGraduation<dwRating; dwGraduation++)
			{
				dwPingAlpha+=40;
				m_pPingIcon->SetPosition(iPosX,iPingPosY);
				m_pPingIcon->SetAlpha(dwPingAlpha);
				m_pPingIcon->Render();

				iPosX += m_pPingIcon->GetWidth() + PING_SPACING;
			}
		}
	}
}