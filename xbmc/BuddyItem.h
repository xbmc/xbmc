#pragma once
#include "guilistexitem.h"
#include "stdstring.h"
#include <vector>
using namespace std;

class CBuddyItem : public CGUIListExItem
{
public:
	CBuddyItem(CStdString& strLabel);
	virtual ~CBuddyItem(void);
	virtual void OnPaint(CGUIItem::RenderContext* pContext);
	void		 SetPingIcon(INT aWidth, INT aHeight, const CStdString& aTexture);
	CStdString	 GetArena();
public:
	CStdString	 m_strVector;
	DWORD		 m_dwPing;
	bool		 m_bIsOnline;
protected:
	CGUIImage*	 m_pPingIcon;
};

