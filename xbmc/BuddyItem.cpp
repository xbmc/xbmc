
#include "stdafx.h"
#include "buddyitem.h"
#include "settings.h"
#include "utils/KaiClient.h"

#define PING_OFFSETX		8
#define PING_MAX_RATING		4
#define PING_SPACING		2
#define PING_MAX_LATENCY	280

CBuddyItem::CBuddyItem(CStdString& strLabel) : CGUIListExItem(strLabel)
{
	SetCookie( CKaiClient::Item::Player );
	m_bIsOnline=false;
	m_strVector="/";
	m_dwPing = 0;
	m_pPingIcon=NULL;
}

CBuddyItem::~CBuddyItem(void)
{
	if (m_pPingIcon)
	{
		m_pPingIcon->FreeResources();
		delete m_pPingIcon;
	}
}

CStdString CBuddyItem::GetArena()
{
	INT arenaDelimiter = m_strVector.ReverseFind('/')+1;
	return m_strVector.Mid(arenaDelimiter);
}

void CBuddyItem::SetPingIcon(INT aWidth, INT aHeight, const CStdString& aTexture)
{
	if (m_pPingIcon)
	{
		m_pPingIcon->FreeResources();
		delete m_pPingIcon;
	}

	m_pPingIcon = new CGUIImage(0,0,0,0,aWidth,aHeight,aTexture,0x0);
	m_pPingIcon->AllocResources();
}

void CBuddyItem::OnPaint(CGUIItem::RenderContext* pContext)
{
	CGUIListExItem::OnPaint(pContext);

	// safely get a pointer to the derived (subclassed) context
	//CGUIListExItem::RenderContext* pDC = dynamic_cast<CGUIListExItem::RenderContext*>(pContext);

	CGUIListExItem::RenderContext* pDC = (CGUIListExItem::RenderContext*)pContext;
	if ( (pDC) && (m_pPingIcon) && (m_dwPing>0) && (m_dwPing<=PING_MAX_LATENCY) )
	{
		DWORD dwPosY = pDC->m_dwPositionY;
		dwPosY += (pDC->m_pButton->GetHeight() - m_pPingIcon->GetHeight())/2;

		DWORD dwPosX = pDC->m_dwPositionX + pDC->m_pButton->GetWidth();
		dwPosX -= ( (m_pPingIcon->GetWidth()+PING_SPACING) * PING_MAX_RATING ) + PING_OFFSETX;

		DWORD dwStep = PING_MAX_LATENCY / PING_MAX_RATING;
		DWORD dwRating = PING_MAX_RATING - (m_dwPing/dwStep);

		for (DWORD dwGraduation=0; dwGraduation<dwRating; dwGraduation++)
		{
			m_pPingIcon->SetPosition(dwPosX,dwPosY);
			m_pPingIcon->Render();

			dwPosX += m_pPingIcon->GetWidth() + PING_SPACING;
		}
	}
}