
#include "stdafx.h"
#include "playlist.h"
#include "util.h"


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
  m_bPlayed = false;
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
  m_iUnplayedItems = -1;
  m_bShuffled = false;
}

CPlayList::~CPlayList(void)
{
  Clear();
}

void CPlayList::Add(CPlayListItem& item)
{
  // set the order identifier to the size of the vector
  item.m_iOrder = m_vecItems.size();
  m_vecItems.push_back(item);

  // increment the unplayed song count
  if (m_iUnplayedItems < 0)
    m_iUnplayedItems = 1;
  else
    m_iUnplayedItems++;
}

void CPlayList::Clear()
{
  m_vecItems.erase(m_vecItems.begin(), m_vecItems.end());
  m_strPlayListName = "";
  m_iUnplayedItems = -1;
  m_bShuffled = false;
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

  // the list is now shuffled!
  m_bShuffled = true;
}

void CPlayList::UnShuffle()
{
  // iterate through the vector swapping items until original order is returned
  for (int curIndex = 0; curIndex < size(); curIndex++)
  {
    //CLog::Log(LOGDEBUG,"UNSHUFFLE:: iteration = %i",curIndex);

    // while the current item is not where it should be...
    while (curIndex != m_vecItems[curIndex].m_iOrder)
    {
      int iDest = m_vecItems[curIndex].m_iOrder;
      //CLog::Log(LOGDEBUG,"  mismatch %i != %i",curIndex,iDest);

      // copy item in destination spot
      CPlayListItem tempItem = m_vecItems[iDest];

      // move item in current location to destination spot
      m_vecItems[iDest]    = m_vecItems[curIndex];
      m_vecItems[curIndex] = tempItem;
    }
  }

  // the list is now unshuffled!
  m_bShuffled = false;
}

const CStdString& CPlayList::GetName() const
{
  return m_strPlayListName;
}

void CPlayList::Remove(const CStdString& strFileName)
{
  int iOrder = -1;
  ivecItems it;
  it = m_vecItems.begin();
  while (it != m_vecItems.end() )
  {
    CPlayListItem& item = *it;
    if (item.GetFileName() == strFileName)
    {
      // decrement unplayed count
      if (!item.WasPlayed())
        m_iUnplayedItems--;

      iOrder = item.m_iOrder;
      it = m_vecItems.erase(it);
      //CLog::Log(LOGDEBUG,"PLAYLIST, removing item at order %i", iPos);
    }
    else
      ++it;
  }
  FixOrder(iOrder);
}

void CPlayList::FixOrder(int iOrder)
{
  if (iOrder < 0) return;

  // the last item was removed
  if (iOrder == (size() + 1)) return;

  // fix all items with a pos higher than position
  ivecItems it;
  it = m_vecItems.begin();
  while (it != m_vecItems.end() )
  {
    CPlayListItem& item = *it;
    if (item.m_iOrder > iOrder)
    {
      //CLog::Log(LOGDEBUG,"  fixing item at order %i", item.m_iOrder);
      item.m_iOrder--;
    }
    ++it;
  }
}

// remove item from playlist by position
void CPlayList::Remove(int position)
{
  int iOrder = -1;
  if (position < (int)m_vecItems.size())
  {
    iOrder = m_vecItems[position].m_iOrder;
    if (!m_vecItems[position].WasPlayed())
      m_iUnplayedItems--;
    m_vecItems.erase(m_vecItems.begin() + position);
  }
  FixOrder(iOrder);
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
    if ( item.IsCDDA() || item.IsOnDVD() )
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

  int iOrder = -1;
  if (!IsShuffled())
  {
    // swap the ordinals before swapping the items!
    //CLog::Log(LOGDEBUG,"PLAYLIST swapping items at orders (%i, %i)",m_vecItems[position1].m_iOrder,m_vecItems[position2].m_iOrder);
    iOrder = m_vecItems[position1].m_iOrder;
    m_vecItems[position1].m_iOrder = m_vecItems[position2].m_iOrder;
    m_vecItems[position2].m_iOrder = iOrder;
  }

  // swap the items
  CPlayListItem anItem = m_vecItems[position1];
  m_vecItems[position1] = m_vecItems[position2];
  m_vecItems[position2] = anItem;
  return true;
}

void CPlayList::SetPlayed(int iItem)
{
  m_vecItems[iItem].SetPlayed();
  m_iUnplayedItems--;
}

void CPlayList::ClearPlayed()
{
  for (int i = 0; i < (int)m_vecItems.size(); i++)
    m_vecItems[i].ClearPlayed();
  m_iUnplayedItems = m_vecItems.size();
}

int CPlayList::GetUnplayed()
{
  return m_iUnplayedItems;
}
