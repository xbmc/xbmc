#include "stdafx.h"
#include "fileitem.h"
#include "settings.h"

CFileItem::CFileItem(const CSong& song)
{
	Clear();
	m_strLabel=song.strTitle;
	m_strPath=song.strFileName;
	m_musicInfoTag.SetSong(song);
	m_lStartOffset = song.iStartOffset;
	m_lEndOffset = song.iEndOffset;
}


CFileItem::CFileItem(const CAlbum& album)
{
	Clear();
	m_strLabel=album.strAlbum;
	m_strPath=album.strPath;
	m_bIsFolder=true;
	m_strLabel2=album.strArtist;
	m_musicInfoTag.SetAlbum(album);
}

CFileItem::CFileItem(const CPlayList::CPlayListItem& item)
{
	Clear();
	m_strLabel=item.GetDescription();
	m_strPath=item.GetFileName();
	m_lStartOffset = item.GetStartOffset();
	m_lEndOffset = item.GetEndOffset();
	m_musicInfoTag = item.GetMusicTag();
}

CFileItem::CFileItem(const CFileItem& item)
{
	*this=item;
}

CFileItem::CFileItem(void)
{
	Clear();
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
	m_iprogramCount = 0;
	m_idepth = 1;
	m_lStartOffset = 0;
	m_lEndOffset = 0;
  m_iLockMode = LOCK_MODE_EVERYONE;
  m_strLockCode = "";
  m_iBadPwdCount = 0;
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
	m_lStartOffset = item.m_lStartOffset;
	m_lEndOffset = item.m_lEndOffset;
	m_fRating=item.m_fRating;
	m_strDVDLabel=item.m_strDVDLabel;
	m_iprogramCount=item.m_iprogramCount;
  m_iLockMode=item.m_iLockMode;
  m_strLockCode=item.m_strLockCode;
  m_iBadPwdCount=item.m_iBadPwdCount;
	return *this;
}

void CFileItem::Clear()
{
	m_strLabel2="";
	m_strLabel="";
	m_pThumbnailImage=NULL;
	m_pIconImage=NULL;
	m_bSelected=false;
	m_strIcon="";
	m_strThumbnailImage="";
	m_musicInfoTag.SetLoaded(false);
	m_fRating=0.0f;
	m_strDVDLabel="";
	m_strPath = "";
	m_strDVDLabel="";
	m_fRating=0.0f;
	m_dwSize=0;
	m_bIsFolder=false;
	m_bIsShareOrDrive=false;
	memset(&m_stTime,0,sizeof(m_stTime));
	m_iDriveType = SHARE_TYPE_UNKNOWN;
	m_lStartOffset = 0;
	m_lEndOffset = 0;
	m_iprogramCount=0;
}

void CFileItem::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_bIsFolder;
		ar << m_strLabel;
		ar << m_strLabel2;
		ar << m_strThumbnailImage;
		ar << m_strIcon;
		ar << m_bSelected;
		
		ar << m_strPath;
		ar << m_bIsShareOrDrive;
		ar << m_iDriveType;
		ar << m_stTime;
		ar << m_dwSize;
		ar << m_fRating;
		ar << m_strDVDLabel;
		ar << m_iprogramCount;
		ar << m_idepth;
		ar << m_lStartOffset;
		ar << m_lEndOffset;
		ar << m_iLockMode;
		ar << m_strLockCode;
		ar << m_iBadPwdCount;

		ar << m_musicInfoTag;
	}
	else
	{
		ar >> m_bIsFolder;
		ar >> m_strLabel;
		ar >> m_strLabel2;
		ar >> m_strThumbnailImage;
		ar >> m_strIcon;
		ar >> m_bSelected;
		
		ar >> m_strPath;
		ar >> m_bIsShareOrDrive;
		ar >> m_iDriveType;
		ar >> m_stTime;
		ar >> m_dwSize;
		ar >> m_fRating;
		ar >> m_strDVDLabel;
		ar >> m_iprogramCount;
		ar >> m_idepth;
		ar >> m_lStartOffset;
		ar >> m_lEndOffset;
		ar >> m_iLockMode;
		ar >> m_strLockCode;
		ar >> m_iBadPwdCount;

		ar >> m_musicInfoTag;
	}
}

CFileItemList::CFileItemList(VECFILEITEMS& items)
:m_items(items)
{
}

CFileItemList::~CFileItemList()
{
	Clear();
}

VECFILEITEMS& CFileItemList::GetList() 
{
	return m_items;
}

void CFileItemList::Clear()
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

void CFileItemList::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		int i=0;
		if (m_items.size()>0 && m_items[0]->GetLabel()=="..")
			i=1;

		ar << (int)(m_items.size()-i);

		for (i; i<(int)m_items.size(); ++i)
		{
			CFileItem* pItem=m_items[i];
			ar << *pItem;
		}
	}
	else
	{
		int iSize=0;
		ar >> iSize;
		if (iSize<=0)
			return;

		Clear();

		m_items.reserve(iSize);

		for (int i=0; i<iSize; ++i)
		{
			CFileItem* pItem=new CFileItem;
			ar >> *pItem;
			m_items.push_back(pItem);
		}
	}
}
