
#include "stdafx.h"
#include "fileitem.h"
#include "settings.h"


CFileItem::CFileItem(const CFileItem& item)
{
	*this=item;
}

CFileItem::CFileItem(void)
{
  m_strDVDLabel="";
  m_fRating=0.0f;
  m_dwSize=0;
  m_bIsFolder=false;
  m_bIsShareOrDrive=false;
	memset(&m_stTime,0,sizeof(m_stTime));
  m_iDriveType = SHARE_TYPE_UNKNOWN;
}


CFileItem::CFileItem(const CStdString& strLabel)
:CGUIListItem(strLabel)
{
  m_strDVDLabel="";
  m_dwSize=0;
  m_fRating=0.0f;
  m_bIsFolder=false;
  m_bIsShareOrDrive=false;
	memset(&m_stTime,0,sizeof(m_stTime));
  m_iDriveType = SHARE_TYPE_UNKNOWN;
}

CFileItem::~CFileItem(void)
{
}

const CFileItem& CFileItem::operator=(const CFileItem& item)
{
	if (this==&item) return *this;
  m_strLabel2=item.m_strLabel2;
  m_strLabel=item.m_strLabel;
  m_pThumbnailImage=NULL;
  m_pIconImage=NULL;
  m_bSelected=item.m_bSelected;
  m_strIcon=item.m_strIcon;
  m_strThumbnailImage=item.m_strThumbnailImage;


	m_strPath=item.m_strPath;
	m_bIsFolder=item.m_bIsFolder;
	m_iDriveType=item.m_iDriveType;
	m_bIsShareOrDrive=item.m_bIsShareOrDrive;
	memcpy(&m_stTime,&item.m_stTime,sizeof(SYSTEMTIME));
	m_dwSize=item.m_dwSize;
	m_musicInfoTag=item.m_musicInfoTag;
  
  m_fRating=item.m_fRating;
  m_strDVDLabel=item.m_strDVDLabel;
	return *this;
}


CFileItemList::CFileItemList(VECFILEITEMS& items)
:m_items(items)
{
}

CFileItemList::~CFileItemList()
{
	if (m_items.size())
	{
		IVECFILEITEMS i;
		i=m_items.begin(); 
		while (i != m_items.end())
		{
			CFileItem* pItem = *i;
			delete pItem;
			i=m_items.erase(i);
		}
	}
}

VECFILEITEMS& CFileItemList::GetList() 
{
	return m_items;
}