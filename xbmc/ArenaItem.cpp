
#include "stdafx.h"
#include "Arenaitem.h"
#include "settings.h"
#include "utils/KaiClient.h"

CArenaItem::CArenaItem(CStdString& strLabel) : CGUIListExItem(strLabel)
{
	SetCookie( CKaiClient::Item::Arena );
	m_strVector="/";
	m_strDescription="";
	m_strPassword="";
}

CArenaItem::~CArenaItem(void)
{
}

CArenaItem::Tier CArenaItem::GetTier()
{
	int tier = 0;

	for(int characterIndex=0; characterIndex<m_strVector.GetLength(); characterIndex++)
	{
		if (m_strVector[characterIndex]=='/')
		{
			tier++;
		}
	}
	
	if (tier>4)
	{
		tier=4;
	}

	return (CArenaItem::Tier) tier;
}

void CArenaItem::GetTier(CArenaItem::Tier aTier, CStdString& aTierName)
{
	GetTier(aTier, m_strVector, aTierName);
}

void CArenaItem::GetTier(Tier aTier, CStdString aVector, CStdString& aTierName)
{
	int tier = 0;
	int characterIndex = 0;
	CStdString name;

	for(; characterIndex<aVector.GetLength(); characterIndex++)
	{
		if (tier==(int)aTier)
		{
			int nextTier = aVector.Find('/',characterIndex+1);
			if (nextTier>=characterIndex)
			{
				aTierName = aVector.Mid(characterIndex,nextTier-characterIndex);
			}
			else
			{
				aTierName = aVector.Mid(characterIndex);
			}
			break;
		}

		if (aVector[characterIndex]=='/')
		{
			tier++;
		}
	}
}
