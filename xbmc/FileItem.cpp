
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
  m_lStartOffset = 0;
  m_lEndOffset = 0;
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
