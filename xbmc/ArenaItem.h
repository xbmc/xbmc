#pragma once

#include "KaiItem.h"

class CArenaItem : public CKaiItem
{
public:
	enum Tier {Root=0,Platform=1,Genre=2,Game=3,Custom=4};

	CArenaItem(CStdString& strLabel);
	virtual ~CArenaItem(void);

	Tier GetTier();
	virtual void OnPaint(CGUIItem::RenderContext* pContext);
	virtual void GetDisplayText(CStdString& aString);

			void GetTier(CArenaItem::Tier aTier, CStdString& aTierName);

	static	void GetTier(Tier aTier, CStdString aVector, CStdString& aTierName);
	static	void SetIcons(INT aWidth, INT aHeight, const CStdString& aHeadsetTexture);
	static  void FreeIcons();

	CStdString m_strVector;
	CStdString m_strDescription;
	CStdString m_strPassword; // arena password - cached

	int m_nPlayers;
	int m_nPlayerLimit;
	bool m_bIsPersonal; // created by another player
	bool m_bIsPrivate;	// requires a password on entry
protected:
	static CGUIImage*	 m_pPrivateIcon;
};
