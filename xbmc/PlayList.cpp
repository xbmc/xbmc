
#include "stdafx.h"
#include "playlist.h"
#include "util.h"

using namespace PLAYLIST;

CPlayList::CPlayListItem::CPlayListItem()
{
}

CPlayList::CPlayListItem::CPlayListItem(const CStdString& strDescription, const CStdString& strFileName, long lDuration)
{
	m_strDescription = strDescription;
	m_strFilename		 = strFileName;
	m_lDuration			 = lDuration;
}

CPlayList::CPlayListItem::~CPlayListItem()
{
}

void CPlayList::CPlayListItem::SetFileName(const CStdString& strFileName)
{
	m_strFilename=strFileName;
}

const CStdString& CPlayList::CPlayListItem::GetFileName() const
{
	return m_strFilename;
}


void CPlayList::CPlayListItem::SetDescription(const CStdString& strDescription)
{
	m_strDescription=strDescription;
}

const CStdString& CPlayList::CPlayListItem::GetDescription() const
{
	return m_strDescription;
}


void CPlayList::CPlayListItem::SetDuration(long lDuration)
{
	m_lDuration=lDuration;
}

long CPlayList::CPlayListItem::GetDuration() const
{
	return m_lDuration;
}

CPlayList::CPlayList(void)
{
	m_strPlayListName="";
}

CPlayList::~CPlayList(void)
{
	Clear();
}


void CPlayList::Add(const CPlayListItem& item) 
{
	m_vecItems.push_back(item);
}

void CPlayList::Clear()
{
	m_vecItems.erase(m_vecItems.begin(),m_vecItems.end());
}

int	CPlayList::size() const
{
	return m_vecItems.size();
}


const CPlayList::CPlayListItem& CPlayList::operator[] (int iItem) const
{
	return m_vecItems[iItem];
}

void CPlayList::Shuffle()
{
	srand( timeGetTime() );

	int nItemCount = size();

	// iterate through each catalogue item performing arbitrary swaps
	for (int nItem=0; nItem<nItemCount; nItem++)
	{
		int nArbitrary = rand() % nItemCount;

		CPlayListItem anItem = m_vecItems[nArbitrary];
		m_vecItems[nArbitrary] = m_vecItems[nItem];
		m_vecItems[nItem] = anItem;
	}
}

const CStdString&	CPlayList::GetName() const
{
	return m_strPlayListName;
}

void CPlayList::Remove(const CStdString& strFileName)
{
	ivecItems it;
	it=m_vecItems.begin();
	while (it != m_vecItems.end() )
	{
		CPlayListItem& item = *it;
		if (item.GetFileName() == strFileName)
		{
			it=m_vecItems.erase(it);
		}
		else ++it;
	}
}
int CPlayList::RemoveDVDItems()
{
	vector <CStdString> vecFilenames;

	//	Collect playlist items from DVD share
	ivecItems it;
	it=m_vecItems.begin();
	while (it != m_vecItems.end() )
	{
		CPlayListItem& item = *it;
		if ( CUtil::IsCDDA( item.GetFileName() ) || CUtil::IsISO9660( item.GetFileName() ) || CUtil::IsDVD( item.GetFileName() ) )
		{
			vecFilenames.push_back( item.GetFileName() );
		}
		it++;
	}

	//	Delete them from playlist
	int nFileCount = vecFilenames.size();
	if ( nFileCount ) {
		vector <CStdString>::iterator it;
		it=vecFilenames.begin();
		while (it != vecFilenames.end() )
		{
			CStdString& strFilename = *it;
			Remove( strFilename );
			it++;
		}
		vecFilenames.erase( vecFilenames.begin(), vecFilenames.end() );
	}
	return nFileCount;
}
