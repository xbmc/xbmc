#pragma once
#include "StdString.h"
#include "KaiItem.h"

class CBuddyItem : public CKaiItem
{
public:
	CBuddyItem(CStdString& strLabel);
	virtual ~CBuddyItem(void);
	virtual void OnPaint(CGUIItem::RenderContext* pContext);
	static  void SetIcons(INT aWidth, INT aHeight, const CStdString& aHeadsetTexture,
							  const CStdString& aChatTexture, const CStdString& aPingTexture, 
							  const CStdString& aInviteTexture, const CStdString& aBusyTexture,
							  const CStdString& aIdleTexture, const CStdString& aHostTexture,
							  const CStdString& aKeyboardTexture);
	static  void FreeIcons();

	CStdString	 GetArena();

public:
	CStdString	 m_strVector;
	CStdString	 m_strGeoLocation;
	DWORD		 m_dwPing;
	bool		 m_bIsOnline;
	bool		 m_bSpeex;
	bool		 m_bBusy;
	bool		 m_bKeyboard;
	bool		 m_bHeadset;
	bool		 m_bIsTalking;
	bool		 m_bIsContact;
	bool		 m_bProfileRequested;

	DWORD		 m_dwSpeexCounter;
	DWORD		 m_dwRingCounter;
	bool		 m_bInvite;	
	bool		 m_bRingIndicator;

	int			 m_nStatus;

protected:

	static CGUIImage*	 m_pPingIcon;
	static CGUIImage*	 m_pTalkingIcon;
	static CGUIImage*	 m_pHeadsetIcon;
	static CGUIImage*	 m_pInviteIcon;
	static CGUIImage*	 m_pBusyIcon;
	static CGUIImage*	 m_pIdleIcon;
	static CGUIImage*	 m_pHostIcon;
	static CGUIImage*	 m_pKeyboardIcon;
};

