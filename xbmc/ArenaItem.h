#pragma once
#include "guilistexitem.h"
#include "stdstring.h"
#include <vector>
using namespace std;

class CArenaItem : public CGUIListExItem
{
public:
	enum Tier {Root=0,Platform=1,Genre=2,Game=3,Custom=4};

	CArenaItem(CStdString& strLabel);
	virtual ~CArenaItem(void);

	Tier GetTier();
	void GetTier(CArenaItem::Tier aTier, CStdString& aTierName);
	static void GetTier(Tier aTier, CStdString aVector, CStdString& aTierName);

	CStdString m_strVector;
	CStdString m_strDescription;
	CStdString m_strPassword; // arena password - cached

	int m_nPlayers;
	int m_nPlayerLimit;
	bool m_bIsPrivate;
};
