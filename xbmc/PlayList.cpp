
#include "stdafx.h"
#include "playlist.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace PLAYLIST;

CPlayList::CPlayListItem::CPlayListItem() : m_lDuration(0)
{
  m_lStartOffset = 0;
  m_lEndOffset = 0;
}

CPlayList::CPlayListItem::CPlayListItem(const CStdString& strDescription, const CStdString& strFileName, long lDuration, long lStartOffset, long lEndOffset)
{
  m_strLabel = strDescription;
  m_strPath = strFileName;
  m_lDuration = lDuration;
  m_lStartOffset = lStartOffset;
  m_lEndOffset = lEndOffset;
}

CPlayList::CPlayListItem::~CPlayListItem()
{}

void CPlayList::CPlayListItem::SetFileName(const CStdString& strFileName)
{
  m_strPath = strFileName;
}

const CStdString& CPlayList::CPlayListItem::GetFileName() const
{
  return m_strPath;
}


void CPlayList::CPlayListItem::SetDescription(const CStdString& strDescription)
{
  m_strLabel = strDescription;
}

const CStdString& CPlayList::CPlayListItem::GetDescription() const
{
  return m_strLabel;
}


void CPlayList::CPlayListItem::SetDuration(long lDuration)
{
  m_lDuration = lDuration;
}

long CPlayList::CPlayListItem::GetDuration() const
{
  return m_lDuration;
}

void CPlayList::CPlayListItem::SetStartOffset(long lStartOffset)
{
  m_lStartOffset = lStartOffset;
}

long CPlayList::CPlayListItem::GetStartOffset() const
{
  return m_lStartOffset;
}

void CPlayList::CPlayListItem::SetEndOffset(long lEndOffset)
{
  m_lEndOffset = lEndOffset;
}

long CPlayList::CPlayListItem::GetEndOffset() const
{
  return m_lEndOffset;
}

void CPlayList::CPlayListItem::SetMusicTag(const CMusicInfoTag &tag)
{
  m_musicInfoTag = tag;
}

CMusicInfoTag CPlayList::CPlayListItem::GetMusicTag() const
{
  return m_musicInfoTag;
}

CPlayList::CPlayList(void)
{
  m_strPlayListName = "";
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
  m_vecItems.erase(m_vecItems.begin(), m_vecItems.end());
}

int CPlayList::size() const
{
  return m_vecItems.size();
}


const CPlayList::CPlayListItem& CPlayList::operator[] (int iItem) const
{
  return m_vecItems[iItem];
}

CPlayList::CPlayListItem& CPlayList::operator[] (int iItem)
{
  return m_vecItems[iItem];
}

void CPlayList::Shuffle()
{
  srand( timeGetTime() );

  int nItemCount = size();

  // iterate through each catalogue item performing arbitrary swaps
  for (int nItem = 0; nItem < nItemCount; nItem++)
  {
    int nArbitrary = rand() % nItemCount;

    CPlayListItem anItem = m_vecItems[nArbitrary];
    m_vecItems[nArbitrary] = m_vecItems[nItem];
    m_vecItems[nItem] = anItem;
  }
}

const CStdString& CPlayList::GetName() const
{
  return m_strPlayListName;
}

void CPlayList::Remove(const CStdString& strFileName)
{
  ivecItems it;
  it = m_vecItems.begin();
  while (it != m_vecItems.end() )
  {
    CPlayListItem& item = *it;
    if (item.GetFileName() == strFileName)
    {
      it = m_vecItems.erase(it);
    }
    else ++it;
  }
}

// remove item from playlist by position
void CPlayList::Remove(int position)
{
  if (position < (int)m_vecItems.size())
    m_vecItems.erase(m_vecItems.begin() + position);
}
int CPlayList::RemoveDVDItems()
{
  vector <CStdString> vecFilenames;

  // Collect playlist items from DVD share
  ivecItems it;
  it = m_vecItems.begin();
  while (it != m_vecItems.end() )
  {
    CPlayListItem& item = *it;
    if ( item.IsCDDA() || item.IsISO9660() || item.IsDVD() )
    {
      vecFilenames.push_back( item.GetFileName() );
    }
    it++;
  }

  // Delete them from playlist
  int nFileCount = vecFilenames.size();
  if ( nFileCount )
  {
    vector <CStdString>::iterator it;
    it = vecFilenames.begin();
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

bool CPlayList::Swap(int position1, int position2)
{
  if (
    (position1 < 0) ||
    (position2 < 0) ||
    (position1 >= size()) ||
    (position2 >= size())
  )
  {
    return false;
  }
  CPlayListItem anItem = m_vecItems[position1];
  m_vecItems[position1] = m_vecItems[position2];
  m_vecItems[position2] = anItem;
  return true;
}

